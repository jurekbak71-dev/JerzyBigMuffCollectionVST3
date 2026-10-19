# Experimental VST3 builds

Latest: **0.2.0**, for Windows x64 and Linux x86_64. Each ZIP contains the VST3 folder, full project/JUCE sources, licence notices and BUILD-INFO.json identifying the source commit. Adjacent SHA-256 files verify the archives.

0.2.0 adds fourteen nominal circuit modes from thirteen referenced schematics, with Colorsound Supa ToneBender as the default. Six earlier approximate voices remain labelled Legacy. The circuit implementation passed 41 independent ngspice numerical comparisons, but has not been calibrated against hardware. See ../docs/ANALOG_MODEL.md and ../docs/VALIDATION.md.

Linux full pluginval and both platforms' three CTest suites passed. Windows audio-only validation under Wine passed; full Windows GUI validation timed out at Editor. Native FL Studio compatibility remains unconfirmed.

The older 0.1.0 Windows build is retained for rollback. GitHub Actions separately builds native Windows/Linux, but its current run status has not been confirmed through the blocked GitHub API.
