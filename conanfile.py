from conan import ConanFile


class CompressorRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    default_options = {"drogon*:with_sqlite": True}

    def requirements(self):
        self.requires("openssl/3.1.2")
        self.requires("cpr/1.10.4")
        self.requires("fmt/10.2.0")
        self.requires("nlohmann_json/3.11.2")
        self.requires("lexbor/2.3.0")
        self.requires("re2/20230901")
        self.requires("sqlitecpp/3.3.1")
        self.requires("drogon/1.8.6")
        self.requires("argon2/20190702")
        self.requires("jwt-cpp/0.6.0")
        self.requires("sqlite3/3.44.2", override=True)
