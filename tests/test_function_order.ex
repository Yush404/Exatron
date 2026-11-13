fn main() -> int {
    print(double_it(21));
    print(is_even(10));
    return 0;
}

fn double_it(x: int) -> int {
    return x * 2;
}

fn is_even(x: int) -> bool {
    return x % 2 == 0;
}

