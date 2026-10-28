# Simple FTP Download Client in C

This project implements a command-line FTP client in C capable of connecting to remote FTP servers, authenticating users, establishing passive data connections, and downloading files via TCP/IP sockets. Developed as part of the Computer Networks (Redes de Computadores - RCOM) curriculum at FEUP.

## Overview

The application adheres to the File Transfer Protocol (RFC 959) specifications, maintaining two distinct TCP connections during file retrieval:

1. **Control Connection**: Established on port 21 to send commands (`USER`, `PASS`, `PASV`, `RETR`, `QUIT`) and parse server status responses.
2. **Data Connection**: Established dynamically using server-specified parameters following a passive mode request (`PASV`) to stream file payload directly to local disk storage.

## Core Features

* **URL Parsing**: Integrates the `libcurl` URL API to parse RFC 1738 compliant FTP URLs, extracting optional user credentials, hostname, file path, and destination filename.
* **DNS Resolution**: Resolves target domain names to IPv4 network addresses using `gethostbyname()`.
* **Standard FTP Authentication**: Supports both authenticated logins (`user:password`) and anonymous access defaults.
* **Passive Mode (`PASV`) Support**: Parses RFC 959 response strings (`227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)`) to extract the dynamic IP address and compute the target data port ($p_1 \times 256 + p_2$).
* **Binary File Transfer**: Opens a dedicated TCP stream on the negotiated passive port, reads data chunks in binary format, and writes them directly to disk.
* **Clean Session Teardown**: Issues the `QUIT` command on the control socket and closes all open file descriptors upon transfer completion.

## Protocol Flow

```text
Client                                Server
  |                                     |
  | -------- TCP SYN (Port 21) -------> |
  | <------- 220 Service Ready -------- |
  |                                     |
  | -------- USER <username> ---------> |
  | <------- 331 Password Required ---- |
  | -------- PASS <password> ---------> |
  | <------- 230 User Logged In ------- |
  |                                     |
  | -------- PASV --------------------> |
  | <------- 227 Entering Passive Mode  |
  |          (h1,h2,h3,h4,p1,p2) ------ |
  |                                     |
  | --- TCP SYN (h1.h2.h3.h4:Port) ---> |  (Data Connection)
  |                                     |
  | -------- RETR <filepath> ---------> |  (Control Connection)
  | <------- 150 Opening Connection --- |
  |                                     |
  | <======= Binary Data Stream ======= |  (Data Connection)
  |                                     |
  | -------- QUIT --------------------> |
  | <------- 221 Closing Connection --- |
```

## Project Structure

* `project.c`: Main program handling URL parsing, socket orchestration, FTP command sequence, and file I/O.
* `project.h`: Definitions of FTP response codes (`220`, `230`, `331`, `227`, `150`) and data structures.
* `getip.c`: Standalone utility demonstrating hostname resolution using `gethostbyname()`.
* `clientTCP.c`: Standalone reference client demonstrating basic TCP client socket initialization and transmission.

## Requirements

* GCC or Clang compiler
* POSIX-compliant operating system (Linux / macOS)
* `libcurl` development library (used for URL parsing)

On Debian/Ubuntu-based distributions:
```bash
sudo apt-get install build-essential libcurl4-openssl-dev
```

## Compilation

Compile the client using `gcc` and link with `libcurl`:

```bash
gcc -Wall -o download project.c -lcurl
```

## Usage

Run the compiled executable by providing a valid FTP URL:

```bash
./download ftp://[<user>:<password>@]<host>/<path-to-file>
```

### Examples

Anonymous download:
```bash
./download ftp://ftp.up.pt/pub/kodi/timestamp.txt
```

Authenticated download with explicit credentials:
```bash
./download ftp://rcom:rcom@netlab1.fe.up.pt/pipe.txt
```

Large binary download:
```bash
./download ftp://rcom:rcom@netlab1.fe.up.pt/files/crab.mp4
```
