"""Use the C3 ROM loader and a full reset after manual download-mode entry."""
Import("env")
if env.subst("$PIOENV") == "tenstar-c3":
    flags = list(env.get("UPLOADERFLAGS", []))
    flags.insert(0, "--no-stub")
    if "--after" in flags:
        flags[flags.index("--after") + 1] = "watchdog_reset"
    env.Replace(UPLOADERFLAGS=flags)
