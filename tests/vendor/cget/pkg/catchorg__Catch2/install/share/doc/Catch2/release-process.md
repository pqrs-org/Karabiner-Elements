<a id="top"></a>
# How to release

When enough changes have accumulated, it is time to release new version of Catch. This document describes the process in doing so, that no steps are forgotten. Note that all referenced scripts can be found in the `scripts/` directory.

## Necessary steps

These steps are necessary and have to be performed before each new release. They serve to make sure that the new release is correct and linked-to from the standard places.


### Approval testing

Catch's releases are primarily validated against output from previous release, stored in `projects/SelfTest/Baselines`. To validate current sources, build the SelfTest binary and pass it to the `approvalTests.py` script: `approvalTests.py <path/to/SelfTest>`.

There should be no differences, as Approval tests should be updated when changes to Catch are made, but if there are, then they need to be manually reviewed and either approved (using `approve.py`) or Catch requires other fixes.


### Incrementing version number

Catch uses a variant of [semantic versioning](http://semver.org/), with breaking API changes (and thus major version increments) being very rare. Thus, the release will usually increment the patch version, when it only contains couple of bugfixes, or minor version, when it contains new functionality, or larger changes in implementation of current functionality.

After deciding which part of version number should be incremented, you can use one of the `*Release.py` scripts to perform the required changes to Catch.

This will take care of generating the single include header, updating
version numbers everywhere and pushing the new version to Wandbox.


### Release notes

Once a release is ready, release notes need to be written. They should summarize changes done since last release. For rough idea of expected notes see previous releases. Once written, release notes should be placed in `docs/release-notes.md`.


### Commit and push update to GitHub

After version number is incremented, single-include header is regenerated and release notes are updated, changes should be commited and pushed to GitHub.


### Release on GitHub

After pushing changes to GitHub, GitHub release *needs* to be created.
Tag version and release title should be same as the new version,
description should contain the release notes for the current release.
Single header version of `catch.hpp` *needs* to be attached as a binary,
as that is where the official download link links to. Preferably
it should use linux line endings. All non-bundled reporters (Automake,
TAP, TeamCity) should also be attached as binaries, as they are dependent
on a specific version of the single-include header.


## Optional steps

Because Catch's [vcpkg](https://github.com/Microsoft/vcpkg) port updates
itself automagically, there are no optional steps at this time.
