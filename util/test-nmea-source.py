#!/usr/bin/env python3

# Stellarium
# Copyright (C) 2026 Ruslan Kabatsayev
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.

import argparse
import socket
import sys
import time
from datetime import datetime, timezone


def nmea(body):
    checksum = 0
    for char in body:
        checksum ^= ord(char)
    return f"${body}*{checksum:02X}\r\n"


def coordinate(degrees, width):
    value = abs(degrees)
    whole = int(value)
    minutes = (value - whole) * 60
    return f"{whole:0{width}d}{minutes:07.4f}"


def sentences(now, lat, lon, altitude, fix):
    lat_text = coordinate(lat, 2)
    lon_text = coordinate(lon, 3)
    ns = "N" if lat >= 0 else "S"
    ew = "E" if lon >= 0 else "W"
    hms = now.strftime("%H%M%S") + ".00"
    date = now.strftime("%d%m%y")
    is_3d = fix == "3d"

    satellites = (
        ["04", "05", "09", "12", "15", "17", "19", "24"]
        if is_3d else ["04", "05", "09"]
    )
    hdop = "0.9" if is_3d else "1.8"
    pdop = "1.5" if is_3d else "2.5"

    rmc = ",".join([
        "GPRMC", hms, "A", lat_text, ns, lon_text, ew,
        "0.0", "0.0", date, "", "", "A",
    ])

    gga = ",".join([
        "GPGGA", hms, lat_text, ns, lon_text, ew,
        "1", f"{len(satellites):02d}", hdop,
        f"{altitude:.1f}" if is_3d else "",
        "M" if is_3d else "",
        "15.0" if is_3d else "",
        "M" if is_3d else "", "", "",
    ])

    gsa = ",".join([
        "GPGSA", "A", "3" if is_3d else "2",
        *(satellites + [""] * (12 - len(satellites))),
        pdop, hdop, "1.2" if is_3d else "",
    ])

    return "".join(nmea(s) for s in (rmc, gga, gsa)).encode("ascii")


def main():
    parser = argparse.ArgumentParser(description="Simulated NMEA GPS over TCP")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=5000)
    parser.add_argument("--fix", choices=("2d", "3d"), default="3d")
    parser.add_argument("--lat", type=float, default=-8.81035)
    parser.add_argument("--lon", type=float, default=115.16751)
    parser.add_argument("--alt", type=float, default=132.4)
    args = parser.parse_args()

    try:
        while True:
            try:
                print(f"Connecting to {args.host}:{args.port}...",
                      file=sys.stderr)
                with socket.create_connection(
                    (args.host, args.port), timeout=5
                ) as sock:
                    print(f"Connected to {args.host}:{args.port} ({args.fix})",
                          file=sys.stderr)

                    while True:
                        now = datetime.now(timezone.utc)
                        data = sentences(
                            now, args.lat, args.lon, args.alt, args.fix
                        )
                        sock.sendall(data)
                        time.sleep(1)

            except OSError as exc:
                print(f"Connection error: {exc}; retrying...",
                      file=sys.stderr)
                time.sleep(2)

    except KeyboardInterrupt:
        print("\nStopped.", file=sys.stderr)


if __name__ == "__main__":
    main()

