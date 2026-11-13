// EXPECT: might not return a value
fn f(x: int) -> int {
    if (x > 0) {
        return 1;
    }
}

fn main() -> int {
    return f(5);
}

