// SPDX-License-Identifier: GPL-3.0-or-later

#![forbid(unsafe_code)]

use kfaceauth_protocol::{Request, Response};

fn main() {
    let arguments: Vec<String> = std::env::args().collect();
    let request = match arguments.as_slice() {
        [_, command] if command == "capabilities" => Request::Capabilities,
        [_, command] if command == "status" => Request::Status,
        _ => {
            eprintln!("usage: kfaceauthctl <capabilities|status>");
            std::process::exit(2);
        }
    };

    match kfaceauth_daemon::handle_request(request) {
        Response::Capabilities(capabilities) => {
            println!(
                "protocol={}..{} status=supported authentication=unsupported enrollment=unsupported pam=unsupported persistence=unsupported",
                capabilities.protocol_min, capabilities.protocol_max
            );
        }
        Response::Status(_) => {
            println!(
                "engine=skeleton authentication=unsupported enrollment=unsupported pam=unsupported persistence=unsupported"
            );
        }
        Response::Error(_) => {
            eprintln!("native engine skeleton returned a typed error");
            std::process::exit(1);
        }
    }
}
