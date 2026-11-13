// EXPECT: Lexer Error
fn main() -> int {
    let s: string = "unterminated;
    return 0;
}

