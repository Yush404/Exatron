fn abs(x: int) -> int {
    if (x < 0) {
        return -x;
    }
    return x;
}

fn main() -> int {
    let new: int = abs(-7);
    let class: float = 2.5;
    let delete: bool = new > 3;
    print("literals:");
    print(5);
    print(class);
    print(delete);
    print(new);
    print(-class);
    return 0;
}

