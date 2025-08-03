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

    run.exec = ''
      cleanup() {
        kill $ENGINE_PID $GATEWAY_PID 2>/dev/null
        exit
      }
      trap cleanup INT TERM EXIT

      build

      ./build/engine &
      ENGINE_PID=$!

      sleep 2

      ./build/gateway &
      GATEWAY_PID=$!

      wait
    '';
  };
}
