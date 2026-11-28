# Testing Stellarium hardware support

This instruction documents techniques for manual testing of Stellarium's hardware support.  For the
automated tests see [the relevant section in BUILDING.md](BUILDING.md#test-suite).

## GPS (raw, not libgps-based)

This test sends simulated GPS data to Stellarium’s direct serial NMEA reader on Ubuntu 22.04. Qt normally
discovers serial ports through libudev or sysfs. Both miss the simulated port’s `/dev` alias, and a
successful lookup prevents Qt from scanning `/dev`. The Bubblewrap setup forces Qt to fall back to scanning
`/dev`, where it exposes the simulated port as a device node.  It also keeps Stellarium from reaching
`gpsd`, which normally takes priority over direct serial port access.

The test doesn't require `root` privileges, so run it as your normal user.

### Setup

1. Use a Stellarium build with GPS support (`ENABLE_GPS=ON`). Install `bubblewrap` and `socat` packages if
   they are not installed.
2. Copy the saved configuration you want to test into a scratch profile. Substitute the path to your
   config file if it differs from the default Linux path shown here. Stellarium uses the last value it
   reads for a setting, so the appended `[gui]` entry sets the test port in the copy.
   ```bash
   gps_test_dir=$(mktemp -d --tmpdir gps_test.XXXXXXXXXX) &&
     mkdir "$gps_test_dir/profile" &&
     cp ~/.stellarium/config.ini "$gps_test_dir/profile/config.ini" &&
     printf '\n[gui]\ngps_interface=ttyUSB99\n' >> "$gps_test_dir/profile/config.ini"
   ```
3. In the same terminal, start a bridge between TCP port 5000 and a pseudoterminal (PTY). Wait for `socat`
   to report that it is listening.
   ```bash
   socat -d -d "PTY,link=$gps_test_dir/gps-pty,rawer,unlink-close=0" \
     TCP4-LISTEN:5000,bind=127.0.0.1,reuseaddr,fork &
   gps_bridge_pid=$!
   ```
4. Open another terminal and start [the NMEA sender](util/test-nmea-source.py). Assuming that Stellarium's
   source tree is `~/src/stellarium`, the invocation will look like this:
   ```bash
   ~/src/stellarium/util/test-nmea-source.py
   ```
   By default it connects to `127.0.0.1:5000` and simulates a 3D fix near Garuda Wisnu Kencana. You can
   change the fix type and location with the sender’s command-line options.

### Testing

1. In the first terminal, launch Stellarium through Bubblewrap. Replace `stellarium` with the path to your
test build if it's not accessible via the `PATH` environment variable.

   ```bash
   bwrap --unshare-user --unshare-net \
     --dev-bind / / \
     --dev /dev \
     --dev-bind "$gps_test_dir/gps-pty" /dev/ttyUSB99 \
     --dev-bind-try /dev/dri /dev/dri \
     --bind-try /dev/shm /dev/shm \
     --perms 0000 --tmpfs /sys/class/tty \
     --setenv XDG_CACHE_HOME "$gps_test_dir/cache" \
     -- stellarium --user-dir "$gps_test_dir/profile"
   ```

   <details>
   <summary>How the command works</summary>

   - `--perms 0000 --tmpfs /sys/class/tty` makes the TTY class directory unreadable inside the namespace.
     On Ubuntu 22.04, libudev uses that directory to enumerate TTY devices. When libudev returns no TTY
     entries, Qt tries reading the directory directly. Qt treats even an empty readable directory as a
     successful scan, so mode `0000` makes it fall back to `/dev`.
   - `--dev /dev` mounts a minimal private device tree. The next `--dev-bind` follows the link created by
     `socat` and exposes the PTY as `/dev/ttyUSB99`. Qt’s `/dev` scan accepts `ttyUSB*` device names but
     skips symlinks, so the `socat` link alone would be ignored.
   - `--unshare-net` gives Stellarium a private network namespace. Its TCP connection to `gpsd` then fails,
     even on loopback. The NMEA sender and bridge communicate on the host, and Stellarium reads their data
     from the PTY.
   - `--unshare-user` gives Bubblewrap the privileges it needs to set up the private mounts as a normal user.
   - `--dev-bind / /` makes the host filesystem and its submounts available through a single recursive bind.
   - `--dev-bind-try /dev/dri /dev/dri` exposes the host’s DRI graphics devices. `--bind-try /dev/shm /dev/shm`
     exposes shared memory used by desktop software. Each option skips its path if it is absent on the host.
   - `--setenv XDG_CACHE_HOME "$gps_test_dir/cache"` directs cache writes into the scratch directory.
   - `--` ends the Bubblewrap options. Stellarium’s `--user-dir` selects the scratch profile, including its
     `config.ini`.

   </details>

2. Use the GPS button in the Location dialog and check that Stellarium moves to the simulated location.
   You can restart the sender with different parameters to see how Stellarium reacts. If Stellarium times
   out while the sender is stopped, click the GPS button again after restarting the sender.

### Cleanup

Exit Stellarium to release the private mounts and stop the NMEA sender with Ctrl+C. Use the first
terminal to stop the bridge and remove the scratch directory.

```bash
kill "$gps_bridge_pid"
rm -r -- "${gps_test_dir:?}"
```

## LX200 and NexStar telescopes

Here we'll use the ASCOM OmniSim telescope emulator and wrap its Alpaca API with AlpacaScope to provide
an LX200 or NexStar interface to Stellarium.

### Setup

1. Get AlpacaScope from 10110111's fork: https://github.com/10110111/alpacascope. The upstream version has
   broken LX200 support that doesn't work with Stellarium.
2. Build AlpacaScope from source by running `make` from its source tree. You may need to install `golang-go`
   first.
3. Get OmniSim: https://github.com/ASCOMInitiative/ASCOM.Alpaca.Simulators and extract it.
4. Run OmniSim's `ascom.alpaca.simulators` binary.
5. Run AlpacaScope from the source tree as follows (don't forget to stop the old instance before running a
   new one if you're testing multiple modes):
    * LX200 mode:
      ```bash
      ./dist/alpacascope-2.4.0-linux-amd64 --alpaca-host 127.0.0.1 --alpaca-port 32323 \
                                           --listen-ip 127.0.0.1 --listen-port 4030 \
                                           --mount-type eqn --debug --mode lx200
      ```
    * For NexStar mode, replace `--mode lx200` with `--mode nexstar`.
6. When the emulator and the wrapper have successfully started, run Stellarium

#### TCP connection

Add a telescope in the Stellarium's Telescope Control plugin, with the following settings:
 - **Telescope controlled by:** → *Stellarium, directly (serial port or network)*
 - **Telescope properties** → **Coordinate system** → *Equinox of the date (JNow)*
 - **Device settings** → **Connection:** → *Network (TCP)*
 - **Host:** → `127.0.0.1`
 - **Port:** → 4030
 - **Device model:**
   * To test LX200, choose *Meade LX200 (compatible)*
   * To test NexStar, choose *Celestron NexStar (compatible)*

#### Serial port

To test support for serial port connections to LX200 and NexStar telescopes, you can create a
virtual serial port.

First, use `socat` to create two endpoints of a virtual serial port (make sure that both `/tmp/ttyVA00` and
`/tmp/ttyVB00` don't exist before running the command):
```
socat -d -d pty,link=/tmp/ttyVA00,echo=0,perm=0777 pty,link=/tmp/ttyVB00,echo=0,perm=0777
```
In another terminal, connect AlpacaScope to one of the endpoints of the virtual serial port:
```
socat TCP:127.0.0.1:4030 /tmp/ttyVA00,b115200,raw,echo=0
```
Now you can add a new telescope in Stellarium as before, but choosing

 - **Device settings** → **Connection** → *Serial port*
 - **Device settings** → **Serial port:** → `/tmp/ttyVB00` (type it in manually, it won't be in the list)

### Testing

After the telescope appears in the list of telescopes, **Start** it and try to point it, watching its marker
in the sky move. Check Stellarium log and AlpacaScope log for errors.

When testing multiple connection modes, don't forget to **Stop** the previous telescope, or the new connection
may fail. When switching from serial to TCP, stop the `socat TCP:...` relay as well.

### Cleanup

We haven't altered the OS, so the only cleanup to do is to terminate the `socat` processes, OmniSim and
AlpacaScope, and delete the test telescope entries in Stellarium.
