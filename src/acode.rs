use std::fs;

#[derive(Debug)]
enum Token {
    // Values & Punctuations
    Ocurly, Ccurly,
    Oparen, Cparen,
    Number(f32),
    Text(String),
    ColorDef,
    Vector2Def,

    // Phases
    Init,
    Loop,

    // Objects
    RectObj,
    TextObj,

    // Actions
    Grow,
    Wait,
    Fade,
    Move,
}

pub fn acode_test() {
    let path = "acodes/simple.ac";
    let content = fs::read_to_string(path).unwrap();
    let mut content = content.as_bytes();

    let mut line: usize = 1;
    let mut i: usize = 0;
    let mut tokens = Vec::<Token>::new();

    while i < content.len() {
        let c = content[i] as char;

        match c {
            '(' => {
                tokens.push(Token::Oparen);
                i += 1;
            }
            ')' => {
                tokens.push(Token::Cparen);
                i += 1;
            }
            '{' => {
                tokens.push(Token::Ocurly);
                i += 1;
            }
            '}' => {
                tokens.push(Token::Ccurly);
                i += 1;
            }
            ' ' | '\r' => i += 1,
            '\n' => {
                line += 1;
                i += 1;
            },
            '"' => {
                // Move index by one to skip the open quote
                i += 1;

                let start = i;
                consume_text(&mut content, &mut i);
                let s = str::from_utf8(&content[start..i]).unwrap();
                tokens.push(Token::Text(s.to_string()));

                // Move index by one to skip the open quote
                i += 1;
            }
            'c' => {
                tokens.push(Token::ColorDef);
                i += 1;
            }
            'v' => {
                if let Some(c) = peek(content, i) {
                    if c == '2' {
                        tokens.push(Token::Vector2Def);
                        // Increment twice for 'v' and '2'
                        i += 2;
                    } else {
                        assert!(false, "[L{line}] Unknown vector size: {c}")
                    }
                } else {
                    assert!(false, "[L{line}] Unsure what to do with 'v' on its own...");
                }
            }
            _ => {
                if c.is_alphabetic() {
                    let start = i;
                    consume_alpha(&mut content, &mut i);
                    let s = str::from_utf8(&content[start..i]).unwrap();
                    if let Some(t) = identify_token(s) {
                        tokens.push(t);
                    } else {
                        assert!(false, "Unknown identifier: '{s}'");
                    }
                } else if c.is_ascii_digit() {
                    let start = i;
                    consume_num(&mut content, &mut i);
                    let s = str::from_utf8(&content[start..i]).unwrap();
                    tokens.push(Token::Number(s.parse::<f32>().unwrap()));
                } else {
                    unimplemented!("[L{line}] Unsure what to do with '{c}'");
                }
            }
        }
    }
    println!("{:?}", tokens);
}

fn peek(content: &[u8], curr: usize) -> Option<char> {
    if curr + 1 < content.len() {
        Some(content[curr + 1] as char)
    } else {
        None
    }
}

fn consume_text(content: &[u8], i: &mut usize) {
    while (*i) < content.len() {
        let c = content[*i] as char;
        if c == '"' {
            break;
        }
        (*i) += 1;
    }
}

fn consume_alpha(content: &[u8], i: &mut usize) {
    while (*i) < content.len() {
        let c = content[*i] as char;
        if !c.is_alphabetic() {
            break;
        }
        (*i) += 1;
    }
}

fn consume_num(content: &[u8], i: &mut usize) {
    while (*i) < content.len() {
        let c = content[*i] as char;
        if !c.is_ascii_digit() && c != '.' {
            break;
        }
        (*i) += 1;
    }
}

fn identify_token(s: &str) -> Option<Token> {
    match s {
        "INIT" => Some(Token::Init),
        "LOOP" => Some(Token::Loop),
        "RECT" => Some(Token::RectObj),
        "TEXT" => Some(Token::TextObj),
        "GROW" => Some(Token::Grow),
        "WAIT" => Some(Token::Wait),
        "FADE" => Some(Token::Fade),
        "MOVE" => Some(Token::Move),
        _ => None,
    }
}
