# Crossings

Personal SuperCollider UGen project (github.com/bjarnig/Crossings). Not Conference
Compass work.

## Commits and branches
Plain Conventional Commits, with no Jira ticket and no trailers: `feat: add WavesetDivide`,
`fix: ...`, `docs: ...`. This overrides the global `[TRIX-XXXXX]` commit rule and the
`TRIX-XXXXX` branch rule, which apply only to Conference Compass repos.

History is direct-to-`main`. Run `git pull --rebase` before pushing, since the README
may have been edited on GitHub.

## Build
`build.sh` builds and installs the `.scx`, classes and help into
`~/Library/Application Support/SuperCollider/Extensions/Crossings/`.
`build/` and `install/` are gitignored.
