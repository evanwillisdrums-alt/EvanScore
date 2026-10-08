# Isolated Windows startup layout experiment

This Qt platform-theme helper is built only by the manual startup diagnostic
workflow's `layout` mode. It is not linked into or packaged with EvanScore.
Use the exact Qt version from the app artifact being tested.

The helper keeps Qt's normal platform theme and installs an event filter once
the application exists. Before the workspace's first rendered frame, it logs
toolbar constraints and optionally changes notation-toolbar alignment and
resizability (`left`) or window width (`wide`). `baseline` changes neither.

Each trial must pass the existing sustained-response desktop check for both
an empty workspace and a real score. Trial artifacts are diagnostic evidence,
not app downloads. A passing trial still requires a source change and a clean
native build without this helper before release.
