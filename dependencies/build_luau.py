from buildscripts.utils import *
from pathlib import Path

if __name__ == "__main__":
    change_directory("Luau")

    ensure_directory("cmake")
    change_directory("cmake")

    run_command(["cmake", "..", "-DCMAKE_BUILD_TYPE=Release"])

    run_command(["cmake", "--build", ".", "--target", "Luau.Analysis", "--config", "Release"])
    run_command(["cmake", "--build", ".", "--target", "Luau.Ast", "--config", "Release"])
    run_command(["cmake", "--build", ".", "--target", "Luau.Compiler", "--config", "Release"])
    run_command(["cmake", "--build", ".", "--target", "Luau.Config", "--config", "Release"])
    run_command(["cmake", "--build", ".", "--target", "Luau.VM", "--config", "Release"])

    # TODO: for windows build, we'd need to do something different here
    run_command(["ar", "-x", "./libLuau.Analysis.a"])
    run_command(["ar", "-x", "./libLuau.Ast.a"])
    run_command(["ar", "-x", "./libLuau.Compiler.a"])
    run_command(["ar", "-x", "./libLuau.Config.a"])
    run_command(["ar", "-x", "./libLuau.VM.a"])
    run_command(["ar", "rcs", "libLuau.a"] + [str(p) for p in Path(".").glob("*.o")])
