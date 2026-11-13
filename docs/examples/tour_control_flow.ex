fn main() -> int {
    let score: int = 85;
    if (score >= 90) {
        print("Grade A");
    } else if (score >= 80) {
        print("Grade B");
    } else {
        print("Grade C");
    }

    let i: int = 0;
    while (i < 3) {
        print(i);
        i = i + 1;
    }

    let total: int = 0;
    for (step in 1..4) {
        total = total + step;
    }
    print(total);

    return 0;
}

