# FAQ

## What is the focus of ox?

The primary focus of ox is easy automated testing of OpenXR apps, and programmatic simulation of popular XR devices.

While the architecture [allows users](drivers.md) to add support for real DIY/mainstream headsets and controllers, that is not the primary focus for now.

**ox** is mainly meant as a tool for OpenXR app developers. It is currently not a replacement for Monado, SteamVR or Link.

## What is the long-term mission of ox?

ox's long-term mission is to help create a fully open-source, cross-platform, hackable XR software stack with support for older computers as well as newer ones. To give control back to the user.

## Why not Monado?
ox is a fresh, first-principles attempt at the problem. It has a modular design, is designed for Windows/Linux/Mac from day one, and each component is designed to be very lightweight and easy to understand.

It is possible to use Monado (with a [remote driver](https://monado.pages.freedesktop.org/monado/howto-remote-driver.html)) for automated testing. But Monado is still predominantly focused on Linux, with experimental ports for other platforms.

ox supports Windows, Linux and Mac from day one (including 13 year old Intel Macbooks!). And ox is designed ground-up for easy automated testing and simulation.

## Why not Meta Simulator?
[Meta Simulator](https://developers.meta.com/horizon/documentation/unity/xrsim-intro/) is only meant for Meta devices. It isn't open source, and isn't meant for programmatic device control (it enforces a particular record/replay testing framework).

**ox** is open source, provides programmatic control, and allows you to simulate any device (even custom ones). ox provides an interactive GUI window (similar to Meta Simulator).

## I see code written by AI. Is this AI Slop?
No. I've worked on this codebase for months, and I use AI the way an experienced carpenter uses power tools (instead of drilling/sawing everything by hand). I've been a professional programmer for over 25 years, and I mercilessly review and edit every character that *I* ship (regardless of who wrote it).

I read every line of code like a paranoid micromanager. I've been reading the OpenXR spec on bus rides to-and-from work, so I'm not cluelessly vibe-coding this stuff.

Of course this will have bugs. It still has a lot of deviations from the spec, since it's currently a prototype. But that's because of the scale of the project, not because of AI or laziness on my part.

OpenXR's [core specification](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html) alone (i.e. without extensions) runs into 180 pages with tonnes of edge cases! I've personally wanted a fully open-source OpenXR runtime for a very long time, but (until now) it was fairly insane for a single person to build a fully-compliant OpenXR runtime. I wouldn't have even attempted this without AI power tools.

## Is ox fully compliant with OpenXR?
Not yet. I'm actively working on full spec compliance (in order to pass OpenXR's [Conformance Test Suite](https://github.com/KhronosGroup/OpenXR-CTS)). It's my top priority, because testing tools like ox can't be unreliable.

I've currently built enough to add automated testing for my project (which uses Blender in XR mode). So **ox** actually works, and has already caught regressions.

## Can I use ox today?
Yes, like an early adopter. I already use ox for automated regression testing of [Blender's XR API](https://github.com/cmdr2/blender-xr-regression-tests), to avoid [my project](https://freebirdxr.com) breaking in new Blender releases.

I often use ox in [GUI mode](gui.md) while building new features, since it's tedious to wear/remove the physical headset every time I want to test a small change. I've also pointed AI agents at the ox [REST API](rest_api.md) to let them iteratively build and test code changes, with access to the headset view and controllers.

Fun fact: A couple of college students recently used ox to add OpenXR support for their DIY controllers (built for a college project). This gave their ESP32-based DIY controller immediate access to the vast collection of OpenXR apps and games across Windows, Linux and Mac (atleast, in theory).
