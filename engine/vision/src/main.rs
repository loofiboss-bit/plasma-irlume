// SPDX-License-Identifier: GPL-3.0-or-later

#![forbid(unsafe_code)]

use std::io;
use std::path::PathBuf;

fn main() {
    let arguments: Vec<_> = std::env::args_os().collect();
    let model_root = match arguments.as_slice() {
        [_, flag, root] if flag == "--model-root" => PathBuf::from(root),
        _ => {
            eprintln!("usage: kfaceauth-vision-worker --model-root ABSOLUTE_ROOT");
            std::process::exit(2);
        }
    };
    if !model_root.is_absolute() {
        eprintln!("vision worker initialization failed");
        std::process::exit(2);
    }
    let Ok(provider) = kfaceauth_vision::FakeDeterministicProvider::from_model_root(&model_root)
    else {
        eprintln!("vision worker initialization failed");
        std::process::exit(1);
    };

    let mut input = io::stdin().lock();
    let mut output = io::stdout().lock();
    if let Err(error) =
        kfaceauth_vision::worker::serve_once_with_provider(&mut input, &mut output, &provider)
    {
        eprintln!("vision worker terminated: {error}");
        std::process::exit(1);
    }
}
