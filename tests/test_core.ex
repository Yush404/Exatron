// General-purpose language test for Exatron
fn factorial(n: int) -> int {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

fn main() -> int {
    let base: int = 5;
    let fact5: int = factorial(base);
    print("Factorial of 5:");
    print(fact5);

    let sum: int = 0;
    for (i in 1..6) {
        sum = sum + i;
    }
    print("Sum 1..5:");
    print(sum);

    let count: int = 3;
    while (count > 0) {
        count = count - 1;
    }
    print("Countdown finished:");
    print(count);

    return 0;
}

