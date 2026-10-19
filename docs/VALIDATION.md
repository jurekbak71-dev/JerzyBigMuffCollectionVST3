# Validation status

This is an experimental 0.1.0 development build, not a hardware-verified recreation.

Verified locally on 2026-10-18:

- Linux Release VST3: built; pluginval 1.0.4 strictness 5 passed, including GUI and automation.
- Linux CTest: 2/2 passed (DSP regression and processor state).
- Windows x64 cross build: built using JUCE 7.0.12 and MinGW. The resulting PE DLL imports Windows system libraries only.
- Windows CTest under Wine 10: 2/2 passed.
- Windows pluginval strictness 5 with `--skip-gui-tests`: passed. This does not validate VST3 editor embedding.
- Windows editor: constructed and rendered off-screen by processor_tests under Wine; this does not validate DAW embedding.
- Full Windows pluginval: timed out in Editor after 30 seconds under Wine. Cause remains unresolved. Native MSVC CI and FL Studio compatibility have not been confirmed.

The CTest suites cover 44.1/48/96 kHz, six voicings, finite output, DC decay, channel isolation, reset, state roundtrip, invalid state, concurrent restore/processing, automation, bypass, mono/stereo layouts, oversized blocks and NaN input. They do not establish analog accuracy, perceived sound quality, or compatibility with every host.

GitHub Actions builds native Windows/Linux and includes full pluginval GUI validation; its result must be checked separately. The cloud setup script currently reports failure at the unresolved Windows editor check, rather than hiding it.
