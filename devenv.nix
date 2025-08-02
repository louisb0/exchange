{pkgs, ...}: {
  languages.cplusplus.enable = true;

  packages = with pkgs; [
    alejandra
    statix
    deadnix
    pre-commit
  ];

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
