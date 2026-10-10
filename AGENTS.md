# Notes for AI agents working on this repo

## Keep SKILL_RECOMMENDATIONS.md up to date

While developing, notice when a task would have gone better with a skill
(a packaged set of instructions for a topic) that isn't installed - for example
you had to work something out from scratch, the developer asked about a topic
none of the installed skills covers, or a new area of the project starts
(audio, networking, text rendering, ...).

When that happens, update `SKILL_RECOMMENDATIONS.md` as part of the same task:

- First check the "Already installed" section and the skills folder
  (`~/.claude/skills/`). Don't recommend what an installed skill already covers.
- If a matching recommendation exists, improve that entry instead of adding a
  near-duplicate (sharpen its scope, update its *Why*).
- Otherwise add a new entry in the same format: a **kebab-case-name**, one
  paragraph describing what the skill should cover (specific enough to hand to a
  prompt that writes the skill), and a short *Why* saying what in this project
  showed the need.
- Keep the list ordered by value for the project's next steps.
- When the developer installs a recommended skill, move it to "Already installed".
- Mention the change in your reply in one line, so the developer knows the list grew.

Don't add entries for one-off questions or for things a quick answer covered.
