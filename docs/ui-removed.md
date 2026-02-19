# UI Removed

The built-in dashboard and HTTP bridge (`heidi-dashd`) have been removed from the `heidi-kernel` repository to keep it focused on low-level daemon and IPC functionality.

Dashboards now live in the `heidid/engine` repository.

Telemetry and control are still available via the Unix domain socket IPC.
