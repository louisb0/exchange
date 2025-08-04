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
      entry = "iwyu";
      pass_filenames = false;
    };

    unit-test = {
      enable = true;
      name = "unit-test";
      entry = "rt";
      pass_filenames = false;
    };
  };

  scripts = {
    setup.exec = ''
      cmake -B build -S .
    '';

    build.exec = ''
      setup
      cmake --build build
    '';

    rt.exec = ''
      build
      ctest --test-dir build --output-on-failure
    '';

    clean.exec = ''
      rm -rf build/ .cache/
    '';

    iwyu.exec = ''
      output=$(iwyu_tool.py -p build gateway/ engine/ client/ 2>&1 | grep -v "no private include name for @headername mapping")
      echo "$output"
      echo "$output" | grep -q "should add these lines:" && exit 1 || exit 0
    '';
  };
}
