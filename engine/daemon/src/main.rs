// SPDX-License-Identifier: GPL-3.0-or-later

#![forbid(unsafe_code)]

use std::io;

fn main() {
    let mut input = io::stdin().lock();
    let mut output = io::stdout().lock();
    if let Err(error) = kfaceauth_daemon::serve_once(&mut input, &mut output) {
        eprintln!("native engine skeleton request failed: {error}");
        std::process::exit(1);
    }
}
