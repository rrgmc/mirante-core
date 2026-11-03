#!/bin/sh
# Downloads the prebuilt Qt that Mirante builds against, with aqtinstall.
# Requires uv (https://docs.astral.sh/uv/).
#
# Qt goes into the folder named by the environment variable MIRANTE_QT_ROOT,
# or into .qt/ in the repository if it is not set. Several projects can share
# one Qt by setting MIRANTE_QT_ROOT to the same folder.
#
# The Qt modules that the build's modules need come from the "qt" list of a
# manifest: the path given as the first argument, or ci/modules.json.
#
# Prints the Qt folder to pass to CMake as CMAKE_PREFIX_PATH.

set -eu

QT_VERSION=6.8.3

case "$(uname -s)" in
    Darwin) QT_HOST=mac; QT_ARCH=clang_64; QT_ARCH_DIR=macos; QT_EXTRA_ARCHIVES=; PLUGIN_SUFFIX=dylib ;;
    # Qt for Linux ships its own ICU, which its libraries and tools need.
    Linux)  QT_HOST=linux; QT_ARCH=linux_gcc_64; QT_ARCH_DIR=gcc_64; QT_EXTRA_ARCHIVES=icu; PLUGIN_SUFFIX=so ;;
    *) echo "Unsupported system: $(uname -s). On Windows, use tools/setup-qt.ps1." >&2; exit 1 ;;
esac

REPO=$(cd "$(dirname "$0")/.." && pwd)
QT_ROOT=${MIRANTE_QT_ROOT:-$REPO/.qt}
QT_DIR=$QT_ROOT/$QT_VERSION/$QT_ARCH_DIR

# qtsvg provides the plugin that loads the SVG icons of item types.
# Qt WebEngine shows web items; it needs qtdeclarative, qtwebchannel and
# qtpositioning. qttools has lupdate and lrelease for the translations, and
# qttranslations has Qt's own texts in other languages. qtimageformats has the
# WebP plugin for dropped images. Each part has a file that shows it is there,
# so an older Qt without one of them is completed.
markers="lib/cmake/Qt6WebEngineWidgets bin/lupdate translations/qtbase_pt_BR.qm plugins/imageformats/libqwebp.$PLUGIN_SUFFIX"

# The Qt modules that the manifest asks for, each with the archive that holds it.
MANIFEST=${1:-$REPO/ci/modules.json}
QT_MODULES=
if [ -e "$MANIFEST" ]; then
    QT_MODULES=$(python3 -c 'import json, sys; print(" ".join(json.load(open(sys.argv[1], encoding="utf-8"))["qt"]))' "$MANIFEST")
fi
extra_archives=
for module in $QT_MODULES; do
    case "$module" in
        Pdf|PdfWidgets) archive=qtpdf ;;
        Charts) archive=qtcharts ;;
        Multimedia|MultimediaWidgets) archive=qtmultimedia ;;
        SerialPort) archive=qtserialport ;;
        Core|Gui|Widgets|Network|Svg|WebChannel|WebEngineWidgets|Test|Positioning|Sql|Xml|Concurrent|PrintSupport) archive= ;;
        *) echo "$MANIFEST names the Qt module $module, which tools/setup-qt.sh does not know. Add it to the script." >&2; exit 1 ;;
    esac
    markers="$markers lib/cmake/Qt6$module"
    case " $extra_archives " in
        *" $archive "*) ;;
        *) extra_archives="$extra_archives $archive" ;;
    esac
done

missing=
for marker in $markers; do
    [ -e "$QT_DIR/$marker" ] || missing=yes
done
if [ -n "$missing" ]; then
    if ! command -v uvx >/dev/null 2>&1; then
        echo "uv is required to download Qt. Install it from https://docs.astral.sh/uv/ and run this script again." >&2
        exit 1
    fi
    echo "Installing Qt $QT_VERSION into $QT_ROOT" >&2
    uvx --from aqtinstall aqt install-qt "$QT_HOST" desktop "$QT_VERSION" "$QT_ARCH" -O "$QT_ROOT" --archives qtbase qtsvg qtdeclarative qttools qttranslations $QT_EXTRA_ARCHIVES -m qtwebengine qtwebchannel qtpositioning qtimageformats $extra_archives >&2
fi

echo "$QT_DIR"
