fn add(a: int, b: int) -> int {
    return a + b;
}

fn greet(name: string) -> void {
    print("Welcome,");
    print(name);
}

fn main() -> int {
    greet("Student");
    let res: int = add(15, 27);
    print(res);
    return 0;
}

