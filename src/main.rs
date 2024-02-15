use tokio::io::{AsyncBufReadExt, BufReader};
use tokio::sync::mpsc;
use tokio::process::Command;
use structopt::StructOpt;
use std::io;
use std::io::Write;
use regex::Regex;

#[derive(Debug, StructOpt)]
struct CliArgs {
    #[structopt(long, use_delimiter = true)]
    targets: Vec<String>,
    #[structopt(long, default_value = "10")]
    timeout: u32,
    #[structopt(short, long, default_value = "10")]
    count: u32,
}

#[derive(Debug)]
struct PingInfo {
    bytes_sent: u32,
    icmp_seq: u32,
    ttl: u32,
    time: f32,
}

#[derive(Debug)]
struct PacketStatistics {
    transmitted: u32,
    received: u32,
    loss_percent: f32,
}

#[derive(Debug)]
struct RoundTripStatistics {
    min: f32,
    avg: f32,
    max: f32,
    stddev: f32,
}

#[derive(Debug)]
struct PingReport {
    destination: String,
    pings: Vec<PingInfo>,
    packets: Option<PacketStatistics>,
    trips: Option<RoundTripStatistics>,
}

fn parse_ping_line(line: &str) -> Option<PingInfo> {
    let re = Regex::new(r"^(?P<destination>\S+).*?(\d+) bytes from (?P<source>.*?): icmp_seq=(?P<icmp_seq>\d+) ttl=(?P<ttl>\d+) time=(?P<time>[\d.]+) ms$").unwrap();
    if let Some(captures) = re.captures(line) {
        let bytes_sent = captures[2].parse().ok()?;
        let icmp_seq = captures["icmp_seq"].parse().ok()?;
        let ttl = captures["ttl"].parse().ok()?;
        let time = captures["time"].parse().ok()?;
        Some(PingInfo {
            bytes_sent,
            icmp_seq,
            ttl,
            time,
        })
    } else {
        None
    }
}

fn parse_ping_statistics(line: &str) -> Option<PacketStatistics> {
    let patterns = [
        r"^.*?(\d+) packets transmitted, (\d+) packets received, ([0-9.]+)% packet loss$",
        r"(\d+) packets transmitted, (\d+) received, (\d+)% packet loss, time (\d+)ms",
    ];
    for pattern in patterns {
        if let Some(captures) = Regex::new(pattern).unwrap().captures(line) {
            let transmitted = captures[1].parse().ok()?;
            let received = captures[2].parse().ok()?;
            let loss_percent = captures[3].parse().ok()?;
            return Some(PacketStatistics {
                transmitted,
                received,
                loss_percent,
            })
        }
    }
    None
}

fn parse_round_trip_statistics(line: &str) -> Option<RoundTripStatistics> {
    let patterns = [
        r"^.*?min/avg/max/stddev = ([0-9.]+)/([0-9.]+)/([0-9.]+)/([0-9.]+) ms$",
        r"^.*?min/avg/max/mdev = ([0-9.]+)/([0-9.]+)/([0-9.]+)/([0-9.]+) ms$",
    ];
    for pattern in patterns {
        if let Some(captures) = Regex::new(pattern).unwrap().captures(line) {
            let min = captures[1].parse().ok()?;
            let avg = captures[2].parse().ok()?;
            let max = captures[3].parse().ok()?;
            let stddev = captures[4].parse().ok()?;
            return Some(RoundTripStatistics { min, avg, max, stddev })
        }
    }
    None
}

async fn execute_ping(target: String, count: u32, timeout: u32, sender: mpsc::Sender<String>) -> Result<PingReport, io::Error> {
    // let command = format!("ping -c {} {}", count, timeout, target);
    let command = format!("ping -c {} {}", count, target);
    // println!("{}", &command);
    let mut child = Command::new("sh")
        .arg("-c")
        .arg(&command)
        .stdout(std::process::Stdio::piped())
        .spawn()?;
    let mut pings = Vec::new();
    let mut packets = None;
    let mut trips = None;
    if let Some(stdout) = child.stdout.take() {
        let reader = BufReader::new(stdout);
        let mut lines = reader.lines();
        while let Some(line) = lines.next_line().await? {
            let message = format!("{} {}", target, line);
            let _ = sender.send(message).await;
            if let Some(statistics) = parse_ping_line(&line) {
                // println!("{:#?}", statistics);
                pings.push(statistics);
            }
            else if let Some(statistics) = parse_ping_statistics(&line) {
                // println!("{:#?}", statistics);
                packets = Some(statistics);
            }
            else if let Some(statistics) = parse_round_trip_statistics(&line) {
                // println!("{:#?}", statistics);
                trips = Some(statistics);
            }
        }
    }
    let status = child.wait().await?;
    if !status.success() {
        return Err(io::Error::new(
            io::ErrorKind::Other,
            format!("{} failed with exit code: {}", target, status),
        ));
    }
    Ok(PingReport {
        destination: target,
        pings,
        packets,
        trips,
    })
}

fn print_results(results: Vec<PingReport>) {
    for item in results {
        println!("{}:", item.destination);
        match item.packets {
            Some(packets) => {
                println!("  Sent: {} Received: {} Loss: {}%", packets.transmitted, packets.received, packets.loss_percent);  
            },
            None => (),
        }
        match item.trips {
            Some(trips) => {
                println!("  Min: {} Avg: {} Max: {} Std: {}", trips.min, trips.avg, trips.max, trips.stddev);
            },
            None => (),
        }
        println!("");
    }
}

async fn launch_workers(args: CliArgs) -> io::Result<()> {
    let mut tasks = Vec::new();
    let (sender, mut receiver) = mpsc::channel::<String>(10);
    for target in args.targets.clone() {
        let sender_clone = sender.clone();
        let task = tokio::spawn(execute_ping(target.clone(), args.count, args.timeout, sender_clone));
        tasks.push(task);
    }
    let hubmsg = tokio::spawn(async move {
        let total: usize = ((args.count + 3) as usize * args.targets.len());
        let mut counter = 0;
        while let Some(message) = receiver.recv().await {
            let percentage = counter as f32 / total as f32 * 100.0;
            print!("\r{:.1}%", percentage);
            io::stdout().flush().unwrap();
            if counter == total {
                break;
            }
            counter += 1;
        }    
    });
    let mut results = Vec::new();
    for task in tasks {
        let res = task.await?;
        match res {
            Ok(values) => {
                results.push(values);
            },
            Err(err) => {
                println!("Error {:?}", err);
            },
        }
    }
    hubmsg.abort();
    println!("\n");
    print_results(results);
    Ok(())
}

#[tokio::main]
async fn main() -> io::Result<()> {
    let args = CliArgs::from_args();
    launch_workers(args).await?;
    Ok(())
}
