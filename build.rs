fn main() {
    println!("cargo::rustc-link-lib=static=raylib");
    println!("cargo::rustc-link-search=raylib-5.5/lib");
}
