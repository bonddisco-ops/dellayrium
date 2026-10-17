# Dellayrium

A spectral delay that looks like an EQ, by Primal Rage Lab.

Every bell on the screen is a frequency band with its own delay: time, feedback, mode
(Mono / Ping-Pong / Wide / Spectral Wide), duck, drive, size and diffuse. Up to 8 bands,
each running either the Analog engine (filter + delay, zero latency) or the FFT engine
(steep band edges, glassy repeats). A waterfall analyzer shows the echoes ringing out.

Builds for Windows and macOS (VST3, AU, standalone) run on GitHub Actions and land in the
`latest` release. See `Installer/HOW TO INSTALL.txt`.
