# Gate 2: Check for emojis in recent commits
echo "Checking for emojis in commits..."
FOUND=0
for commit in $(git log --format=%H -5 HEAD 2>/dev/null || echo "HEAD"); do
  MSG=$(git log -1 --format=%B "$commit" 2>/dev/null || echo "")
  if echo "$MSG" | grep -q $'[\xf0\x9f\x98\x80-\xf0\x9f\xbf\xbf]' 2>/dev/null; then
    warn "Emoji found in commit $commit"
    FOUND=1
  fi
done
if [[ "$FOUND" == "1" ]]; then
  fail "Emojis found in commits - remove them before pushing"
fi
pass "No emojis in recent commits"