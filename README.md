# DesktopGit

DesktopGit is a small desktop Git client built with C++ and Qt Quick.

The goal of this project is to make common Git actions easier to see and use from a clean GUI. The app works with local repositories and uses the system `git` command line tool under the hood.

## Tech Stack

- C++20
- Qt 6 / Qt Quick / QML
- Qt Quick Controls 2
- CMake
- Git CLI through `QProcess`
- Qt Test for core tests
- Lucide SVG icons

## Requirements

To build and use the project, you need:

- Qt 6.5 or newer
- CMake 3.21 or newer
- A C++20 compiler
- Git installed and available in `PATH`

For normal GitHub work, your system Git must already be configured. For example, SSH keys or HTTPS credentials should work in the terminal before using push, pull, fetch, or clone inside the app.

## Features

- Open an existing local Git repository
- Initialize a local Git repository
- Connect a local repository to a remote URL
- Clone a repository into a selected folder
- View changed files
- View readable file diffs with added and removed lines
- Stage and unstage one or many files
- Select all changed files
- Create commits with a custom commit message
- Push commits to the remote repository
- Fetch and pull remote changes
- Show local branch state with ahead / behind information
- View local branches
- Create and switch branches
- View commit history
- View commit details, changed files, and commit diffs
- Use stash actions: push, apply, pop, and drop
- Dark UI with SVG icons

## Build

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/desktop_git
```

## Run Tests

```bash
ctest --test-dir build --output-on-failure
```

## Screenshots

### Changes And Diff

![Changes and diff view](docs/screenshots/changes-view.jpg)

### Commit History

![Commit history view](docs/screenshots/history-view.jpg)

## Notes

This project does not use `libgit2`. It calls the installed `git` executable with `QProcess`. This keeps the project simpler and makes the behavior close to normal terminal Git.

Icons are stored in `assets/icons/lucide`. Their license information is in `assets/icons/LICENSES.md`.
