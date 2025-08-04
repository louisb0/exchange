{pkgs, ...}: {
  languages.cplusplus.enable = true;

  packages = with pkgs; [
    alejandra
    include-what-you-use
    clang-tools
  ];

  git-hooks.hooks = {
    alejandra = {
      enable = true;
      settings.check = true;
    };

    clang-format = {
      enable = true;
      entry = "${pkgs.clang-tools}/bin/clang-format --dry-run -Werror";
    };

    clang-tidy = {
      enable = true;
      entry = "${pkgs.clang-tools}/bin/clang-tidy -p build";
    };

    iwyu = {
      enable = true;
      name = "include-what-you-use";
      entry = "iwyu_tool.py -p build";
      files = "\\.(cpp|hpp)$";
      pass_filenames = false;
    };
  };

  scripts = {
    setup.exec = ''
      cmake -B build -S .
    '';

    build.exec = ''
      cmake --build build
    '';

    clean.exec = ''
      rm -rf build/ .cache/
    '';
  };
}
