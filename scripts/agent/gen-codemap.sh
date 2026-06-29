#!/usr/bin/env bash
# Deterministic codemap generator (no LLM). Usage: gen-codemap.sh <repo>
set -uo pipefail
REPO="${1:?usage: gen-codemap.sh <repo>}"
[ -d "$REPO/.git" ] || { echo "  skip (not a repo): $REPO"; exit 0; }
cd "$REPO"; NAME=$(basename "$REPO"); mkdir -p docs/agent; OUT="docs/agent/codemap.md"
TYPE=""
[ -f go.mod ] && TYPE="$TYPE Go"
[ -f pubspec.yaml ] && TYPE="$TYPE Flutter/Dart"
[ -f platformio.ini ] && TYPE="$TYPE PlatformIO-firmware"
[ -f package.json ] && TYPE="$TYPE Node"
if [ -f astro.config.mjs ] || [ -f astro.config.ts ]; then TYPE="$TYPE Astro"; fi
[ -f Makefile ] && TYPE="$TYPE Make"
[ -z "$TYPE" ] && TYPE=" (unknown)"
{
echo "# ${NAME} - Code Map"
echo
echo "_Auto-generated (deterministic) by .agent-kit/gen-codemap.sh. Re-run to refresh; edit prose only OUTSIDE the GENERATED block._"
echo
echo "- **Type:**${TYPE}"
echo "- **Branch:** $(git branch --show-current 2>/dev/null)"
echo
echo "<!-- BEGIN GENERATED -->"
echo; echo "## Top-level layout"; echo; echo '```'
for d in */; do d="${d%/}"; case "$d" in .git|node_modules|_references|refs|build|.dart_tool|dist|vendor|.svelte-kit|.github) continue;; esac; [ -d "$d" ] || continue
  printf '%-26s %5s files\n' "$d/" "$(find "$d" -type f -not -path '*/.git/*' 2>/dev/null | wc -l | tr -d ' ')"; done
echo '```'
echo; echo "## Source distribution (files per directory)"; echo; echo '```'
find . -type f \( -name '*.go' -o -name '*.dart' -o -name '*.kt' -o -name '*.ts' -o -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.py' -o -name '*.svelte' -o -name '*.astro' \) \
  -not -path '*/.git/*' -not -path '*/_references/*' -not -path '*/node_modules/*' -not -path '*/build/*' 2>/dev/null \
  | sed 's#/[^/]*$##; s#^\./##' | sort | uniq -c | sort -rn | head -18
echo '```'
echo; echo "## Entry points / key files"; echo
f=0; for e in main.go cmd/*/main.go lib/main.dart src/main.ts firmware/application/src/app_main.c platformio.ini Makefile pubspec.yaml package.json go.mod CLAUDE.md; do [ -e "$e" ] && { echo "- \`$e\`"; f=1; }; done
[ "$f" = 0 ] && echo "- (none auto-detected)"
echo; echo "<!-- END GENERATED -->"
} > "$OUT"
echo "  wrote $REPO/$OUT"
