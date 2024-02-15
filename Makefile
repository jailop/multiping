release:
	cargo build --release

dist-macos-aarch64: release
	cd target/release
	zip ../../multiping-macos-aarch64.zip multiping
	cd ../../
