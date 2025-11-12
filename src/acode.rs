use std::fs;

use crate::rl::{Color, Vector2};

type Number = f32;

#[derive(Clone, Debug, PartialEq)]
enum Token {
    // Values & Punctuations
    Ocurly, Ccurly,
    Oparen, Cparen,
    Semicolon,
    Number(Number),
    Text(String),
    ColorDef,
    Vector2Def,
    On, Off,

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

struct Lexer {
    source: Vec<u8>,
    line: usize,
}

impl Lexer {
    fn from_file(path: &str) -> Self {
        let content = fs::read_to_string(path).unwrap();
        Self {
            source: content.as_bytes().to_owned(),
            line: 1,
        }
    }

    fn tokenize(&mut self) -> Vec<Token> {
        let mut tokens = Vec::<Token>::new();
        let mut i: usize = 0;
        while i < self.source.len() {
            let c = self.source[i] as char;

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
                    self.line += 1;
                    i += 1;
                },
                ';' => {
                    tokens.push(Token::Semicolon);
                    i += 1;
                }
                '"' => {
                    // Move index by one to skip the open quote
                    i += 1;

                    let start = i;
                    self.consume_text(&mut i);
                    let s = str::from_utf8(&self.source[start..i]).unwrap();
                    tokens.push(Token::Text(s.to_string()));

                    // Move index by one to skip the open quote
                    i += 1;
                }
                'c' => {
                    tokens.push(Token::ColorDef);
                    i += 1;
                }
                '-' => {
                    if let Some(c) = self.peek(i) {
                        if c.is_ascii_digit() {
                            // Move on from the negative symbol
                            i += 1;
                            // Parse as negative number as positive
                            let start = i;
                            self.consume_num(&mut i);
                            let s = str::from_utf8(&self.source[start..i]).unwrap();
                            // Multiply the final number with -1.0
                            tokens.push(Token::Number(-1.0 * s.parse::<Number>().unwrap()));
                        } else {
                            assert!(false, "[L{}] Unknown vector size: {c}", self.line)
                        }
                    } else {
                        assert!(false, "[L{}] Unsure what to do with 'v' on its own...", self.line);
                    }
                }
                'v' => {
                    if let Some(c) = self.peek(i) {
                        if c == '2' {
                            tokens.push(Token::Vector2Def);
                            // Increment twice for 'v' and '2'
                            i += 2;
                        } else {
                            assert!(false, "[L{}] Unknown vector size: {c}", self.line)
                        }
                    } else {
                        assert!(false, "[L{}] Unsure what to do with 'v' on its own...", self.line);
                    }
                }
                _ => {
                    if c.is_alphabetic() {
                        let start = i;
                        self.consume_alpha(&mut i);
                        let s = str::from_utf8(&self.source[start..i]).unwrap();
                        if let Some(t) = self.identify_token(s) {
                            tokens.push(t);
                        } else {
                            assert!(false, "Unknown identifier: '{s}'");
                        }
                    } else if c.is_ascii_digit() {
                        let start = i;
                        self.consume_num(&mut i);
                        let s = str::from_utf8(&self.source[start..i]).unwrap();
                        tokens.push(Token::Number(s.parse::<Number>().unwrap()));
                    } else {
                        unimplemented!("[L{}] Unsure what to do with '{c}'", self.line);
                    }
                }
            }
        }

        tokens
    }

    fn peek(&self, curr: usize) -> Option<char> {
        if curr + 1 < self.source.len() {
            Some(self.source[curr + 1] as char)
        } else {
            None
        }
    }

    fn consume_text(&mut self, i: &mut usize) {
        while (*i) < self.source.len() {
            let c = self.source[*i] as char;
            if c == '"' {
                break;
            }
            (*i) += 1;
        }
    }

    fn consume_alpha(&mut self, i: &mut usize) {
        while (*i) < self.source.len() {
            let c = self.source[*i] as char;
            if !c.is_alphabetic() {
                break;
            }
            (*i) += 1;
        }
    }

    fn consume_num(&mut self, i: &mut usize) {
        while (*i) < self.source.len() {
            let c = self.source[*i] as char;
            if !c.is_ascii_digit() && c != '.' {
                break;
            }
            (*i) += 1;
        }
    }

    fn identify_token(&self, s: &str) -> Option<Token> {
        match s {
            "INIT" => Some(Token::Init),
            "LOOP" => Some(Token::Loop),
            "RECT" => Some(Token::RectObj),
            "TEXT" => Some(Token::TextObj),
            "GROW" => Some(Token::Grow),
            "WAIT" => Some(Token::Wait),
            "FADE" => Some(Token::Fade),
            "MOVE" => Some(Token::Move),
            "ON" => Some(Token::On),
            "OFF" => Some(Token::Off),
            _ => None,
        }
    }
}

enum Phase {
    Init,
    Loop
}

struct Parser {
    tokens: Vec<Token>,
}

#[derive(Debug)]
pub enum ObjInfo {
    Rect,
    Text(String)
}

#[derive(Debug)]
pub enum Instruction {
    Create(ObjInfo),
    Grow(usize, f32),
    Wait(f32),
    Fade(usize, Color, f32),
    Move(usize, Vector2, f32),
    Enable(usize),
    Disable(usize),
}

impl Parser {
    fn new(tokens: Vec<Token>) -> Self {
        Self { tokens }
    }

    fn parse(&mut self) -> (Vec<Instruction>, Vec<Instruction>) {
        let mut i: usize = 0;
        let mut setup_insts = Vec::<Instruction>::new();
        let mut loop_insts = Vec::<Instruction>::new();

        while i < self.tokens.len() {
            let t = &self.tokens[i];

            match *t {
                Token::Init => {
                    // Consume token init to move on
                    self.consume_token(&mut i);
                    self.consume_expect(&mut i, &[Token::Ocurly]);

                    while !self.peek_expect(i, &[Token::Ccurly]) {
                        let obj_kind = self.consume_expect(&mut i, &[Token::RectObj, Token::TextObj]);
                        let create_kind = match obj_kind {
                            Token::RectObj => ObjInfo::Rect,
                            Token::TextObj => {
                                let text = self.consume_expect_text(&mut i);
                                ObjInfo::Text(text.to_string())
                            }
                            _ => unreachable!()
                        };

                        setup_insts.push(Instruction::Create(create_kind));
                        self.consume_expect(&mut i, &[Token::Semicolon]);
                    }

                    self.consume_expect(&mut i, &[Token::Ccurly]);
                }
                Token::Loop => {
                    // Consume token loop to move on
                    self.consume_token(&mut i);
                    self.consume_expect(&mut i, &[Token::Ocurly]);

                    while !self.peek_expect(i, &[Token::Ccurly]) {
                        let act_tok = self.consume_expect(&mut i,
                            &[Token::Grow, Token::Move, Token::Fade, Token::Wait, Token::On,
                                Token::Off]
                        );

                        let stmt = match *act_tok {
                            Token::Grow => {
                                let ind = self.consume_expect_number(&mut i);
                                let duration = self.consume_expect_number(&mut i);
                                Instruction::Grow(ind as usize, duration)
                            }
                            Token::Wait => {
                                let duration = self.consume_expect_number(&mut i);
                                Instruction::Wait(duration)
                            }
                            Token::Fade => {
                                let ind = self.consume_expect_number(&mut i);
                                let color = self.consume_color(&mut i);
                                let duration = self.consume_expect_number(&mut i);
                                Instruction::Fade(ind as usize, color, duration)
                            }
                            Token::Move => {
                                let ind = self.consume_expect_number(&mut i);
                                let pos = self.consume_v2(&mut i);
                                let duration = self.consume_expect_number(&mut i);
                                Instruction::Move(ind as usize, pos, duration)
                            }
                            Token::On => {
                                let ind = self.consume_expect_number(&mut i);
                                Instruction::Enable(ind as usize)
                            }
                            Token::Off => {
                                let ind = self.consume_expect_number(&mut i);
                                Instruction::Disable(ind as usize)
                            }
                            _ => {
                                eprintln!("{:?}", loop_insts);
                                unreachable!("Unsure what to do with: {:?}", *act_tok);
                            }
                        };

                        loop_insts.push(stmt);
                        self.consume_expect(&mut i, &[Token::Semicolon]);
                    }

                    self.consume_expect(&mut i, &[Token::Ccurly]);
                }
                _ => unimplemented!("Unsure what to do with this token: {t:?}")
            }
        }

        (setup_insts, loop_insts)
    }

    fn consume_expect(&self, i: &mut usize, expected: &[Token]) -> &Token {
        let t = self.consume_token(i).unwrap();
        assert!(expected.contains(&t), "Expected {:?}, got {:?}", expected, t);
        t
    }

    fn consume_expect_number(&self, i: &mut usize) -> Number {
        let t = self.consume_token(i).unwrap();
        assert!(matches!(t, Token::Number(_)), "Expected Token::Number(_), got {:?}", t);

        match t {
            Token::Number(n) => *n,
            _ => unreachable!(),
        }
    }

    fn consume_expect_text(&self, i: &mut usize) -> &str {
        let t = self.consume_token(i).unwrap();
        assert!(matches!(t, Token::Text(_)));

        match t {
            Token::Text(text) => text,
            _ => unreachable!(),
        }
    }

    fn consume_token(&self, i: &mut usize) -> Option<&Token> {
        if (*i) < self.tokens.len() {
            let t = &self.tokens[*i];
            (*i) += 1;
            Some(t)
        } else {
            None
        }
    }

    fn peek_expect(&self, i: usize, expected: &[Token]) -> bool {
        if i < self.tokens.len() {
            let t = &self.tokens[i];
            expected.contains(t)
        } else {
            false
        }
    }

    fn consume_color(&self, i: &mut usize) -> Color {
        self.consume_expect(i, &[Token::ColorDef]);
        self.consume_expect(i, &[Token::Oparen]);

        let r = self.consume_expect_number(i) as u8;
        let g = self.consume_expect_number(i) as u8;
        let b = self.consume_expect_number(i) as u8;
        let a = self.consume_expect_number(i) as u8;
        self.consume_expect(i, &[Token::Cparen]);

        Color { r, g, b, a }
    }

    fn consume_v2(&self, i: &mut usize) -> Vector2 {
        self.consume_expect(i, &[Token::Vector2Def]);
        self.consume_expect(i, &[Token::Oparen]);

        let x = self.consume_expect_number(i);
        let y = self.consume_expect_number(i);
        self.consume_expect(i, &[Token::Cparen]);

        Vector2 { x, y }
    }
}

pub fn parse(filepath: &str) -> (Vec<Instruction>, Vec<Instruction>) {
    let mut lexer = Lexer::from_file(filepath);
    let tokens = lexer.tokenize();
    // println!("{:?}", tokens);

    let mut parser = Parser::new(tokens);
    parser.parse()
}

pub fn acode_test() {
    parse("acodes/simple.ac");
}
