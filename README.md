# Pintos

Pintos is an instructional operating system framework for the x86 architecture, designed for teaching operating systems concepts. This repository contains the Pintos source code, documentation, and supporting scripts.

## Project Structure

- `src/` — Main source code for the Pintos kernel, user programs, libraries, and utilities.
  - `threads/` — Core kernel and threading implementation.
  - `userprog/` — User program support and system call handling.
  - `filesys/` — File system implementation.
  - `devices/` — Device drivers.
  - `vm/` — Virtual memory subsystem.
  - `lib/`, `misc/`, `examples/`, `tests/`, `utils/` — Libraries, miscellaneous code, example programs, test cases, and utilities.
- `doc/` — Comprehensive documentation and manual for Pintos, including build instructions, project descriptions, and technical references.
- `scripts/` — Helper scripts for building, running, and testing Pintos.
- `specs/` — Hardware and software specifications, including keyboard and VGA documentation.
- `Makefile` — Top-level makefile for building the project.
- `README.md` — This file.

## Documentation

The `doc/` directory contains the full Pintos manual, including:

- Introduction and overview
- Build and run instructions
- Project descriptions (threads, user programs, file system, virtual memory)
- Debugging tips
- Coding standards

To view the manual, open `doc/doc.texi` or the generated HTML files.

## Contributing

This codebase is intended for educational use. If you wish to contribute, please follow the coding standards described in the documentation.

## License

See the `LICENSE` file for licensing information.

---

For more details, consult the manual in the `doc/` directory.
