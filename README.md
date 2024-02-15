# multiping - Ping to multiple sites

It uses the ping command installed in your system to test connections to
simultaneous sites.

It has been tested in Linux and MacOS.

This tool is intended for diagnostic purposes.

## Rust version

To build it:

    $ cargo build

## C version

The C version in under the folder `c-impl`

    $ make

## Usage

To get help:

    $ ./multiping --help

Example:

    $ ./multiping --targets google.com,yahoo.com,localhost -c 25

Write the targets separated by commas, without spaces between them.

With the previous example, the output should be similar to:

    100.0%

    google.com:
      Sent: 25 Received: 25 Loss: 0%
      Min: 196.742 Avg: 202.431 Max: 208.739 Std: 3.317

    yahoo.com:
      Sent: 25 Received: 25 Loss: 0%
      Min: 273.759 Avg: 277.495 Max: 284.014 Std: 3.44

    localhost:
      Sent: 25 Received: 25 Loss: 0%
      Min: 0.083 Avg: 0.166 Max: 0.203 Std: 0.031
