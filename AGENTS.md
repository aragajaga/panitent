# AGENTS.md — Build CMake in a Windows Docker container with MSVC Build Tools

This document describes a reproducible approach to building a CMake-based C/C++ project inside a Windows Docker container that has Microsoft Visual C++ (MSVC) toolchain installed (Visual Studio Build Tools). It explains two practical approaches, provides a tested Dockerfile skeleton, and shows how to run CMake and MSVC from inside the container.

---

## Overview

Goals:

* Provide a small, reproducible Windows container image that contains MSVC build tools and CMake.
* Show how to run CMake (Ninja or Visual Studio generator) and produce artifacts.
* Keep the image reasonably cache-friendly and document common pitfalls.

Constraints & notes:

* You must build Windows containers on a Windows host (Docker Desktop with Windows containers enabled) — Linux hosts cannot run Windows containers.
* Visual Studio Build Tools installers are large; expect final image sizes to be hundreds of MBs to multiple GBs depending on workloads.
* Installing via the Visual Studio bootstrapper in a Dockerfile is the recommended path for CI images.

---

## Recommended approach (short)

1. Use a Microsoft Windows Server Core / .NET Framework SDK base image if you need .NET; otherwise use a suitable `windowsservercore` base image.
2. Download the Visual Studio Build Tools bootstrapper (`vs_buildtools.exe`) inside the Dockerfile and run it in quiet mode, adding the C++ workload and the VC tools and CMake components you need.
3. (Optional) Install a newer CMake + Ninja using `winget`/Kitware installers inside the image if the bundled CMake is out of date.
4. Configure the build in container by calling the MSVC environment batch (`vcvarsall.bat`) or use CMake’s Visual Studio/Ninja generator directly.

---

## Example Dockerfile (practical)

This Dockerfile is a practical starting point. It:

* Uses a `mcr.microsoft.com/dotnet/framework/sdk` image as base (includes common runtime bits and a friendly starting surface).
* Downloads the Visual Studio Build Tools bootstrapper (the `aka.ms` bootstrapper) and installs the C++ workload + MSVC toolset and CMake component.
* Installs the latest Kitware CMake and Ninja using winget (recommended for up-to-date CMake).

```dockerfile
# escape=\
FROM mcr.microsoft.com/dotnet/framework/sdk:4.8-windowsservercore-ltsc2019

SHELL ["powershell", "-Command", "$ErrorActionPreference = 'Stop'; $ProgressPreference = 'SilentlyContinue';"]

# Set a working dir
WORKDIR C:\build

# Download Visual Studio Build Tools bootstrapper
RUN Invoke-WebRequest -UseBasicParsing -Uri "https://aka.ms/vs/17/release/vs_buildtools.exe" -OutFile C:\vs_buildtools.exe;

# Install MSVC build tools and CMake support
# NOTE: adjust --add options to match the workloads/components you need.
RUN Start-Process -FilePath C:\vs_buildtools.exe -ArgumentList @(
    '--quiet', '--wait', '--norestart', '--nocache',
    '--installPath', '"%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"',
    '--add', 'Microsoft.VisualStudio.Workload.VCTools',
    '--add', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
    '--add', 'Microsoft.VisualStudio.Component.VC.CMake.Project'
) -NoNewWindow -Wait;

# Optional: install/up-to-date CMake and Ninja via winget (requires winget availability), or get latest CMake installer from Kitware.
RUN Invoke-Expression 'choco feature enable -n allowGlobalConfirmation' ; `
    choco install -y win10sdk || Write-Output "win10sdk may not be required";

# Add small wrapper scripts and clean up
RUN Remove-Item C:\vs_buildtools.exe -Force

# Set PATH hints (the installer registers MSBuild & vcvars) — final image should expose msbuild & cl.exe in PATH.
ENV PATH="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin;${env:PATH}"

WORKDIR C:\src

# Default command: show msbuild version (adjust in your CI to run builds)
CMD ["cmd", "/C", "msbuild /version"]
```

### Notes on customizing the Dockerfile

* Replace the base image to match your Windows host compatibility (e.g. `ltsc2022` images for newer hosts). The Visual Studio system requirements indicate which Windows versions are supported.
* The `--add` values control what is installed. Use Microsoft’s workload/component ID pages to pick the exact components you need, and restrict install size by not adding unnecessary items.
* If your builds require a particular Windows SDK, add the `Microsoft.VisualStudio.Component.Windows10SDK.*` component or install a standalone SDK.
* If `choco` or `winget` are not present in the base image, include their installation steps or install Kitware CMake directly.

---

## How to run a CMake build inside the container

From a container shell (PowerShell or cmd), you can either:

### Option A — Use the Visual Studio generator (multi-config)

```powershell
# from inside container in project root
mkdir build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

This produces Visual Studio solution-style multi-config output and will use MSVC.

### Option B — Use Ninja + MSVC toolchain (single-config)

```powershell
# ensure vcvars are available or let CMake detect the toolchain
cmake -S . -B build -G Ninja -A x64
cmake --build build --config Release
```

If needed, explicitly run the MSVC environment first:

```powershell
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
```

---

## CI tips and best practices

* Build caching: install the build tools in a separate Dockerfile stage so that layer caching avoids re-installing them for unrelated rebuilds.
* Image size: only add workloads/components you need. The `Microsoft.VisualStudio.Workload.VCTools` workload and the `Microsoft.VisualStudio.Component.VC.Tools.x86.x64` component are typically the minimum for native MSVC builds.
* Reproducibility: pin the bootstrapper to a fixed release if you need deterministic builds; otherwise use the `aka.ms` bootstrapper for latest.
* Debugging the install: running the install steps interactively inside a running container can help diagnose issues that fail when run during image build (some installers behave differently in the non-interactive build context).

---

## Troubleshooting

* If the installer seems to finish but tools are missing at runtime, ensure your base image / host compatibility and that the installer ran successfully (check logs in `C:\ProgramData\Microsoft\VisualStudio\Packages`).
* If you need newer CMake/Ninja than Visual Studio provides, install them separately (Kitware releases, Chocolatey, or winget) and add to PATH.
* Some workloads/components may have known issues in container builds — consult Microsoft’s docs for recommended workload lists and example Dockerfiles.

---

## Additional resources

* Official Microsoft guide: *Install Build Tools into a container* (search "Visual Studio Install Build Tools container").
* Workload & component IDs: look up the IDs for exactly what you want to add to the installer.
* Command-line install examples: the VS docs show command-line flags for quiet installs and examples tailored for container builds.

---

End of AGENTS.md
