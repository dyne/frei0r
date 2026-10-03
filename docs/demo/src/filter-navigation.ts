export function navigateFilterKey(
  key: string,
  focusedIndex: number,
  visibleIndexes: readonly number[],
  selectFilter: (index: number) => void
) {
  const position = visibleIndexes.indexOf(focusedIndex)
  if (position < 0 || visibleIndexes.length === 0) return false

  let nextIndex: number | undefined
  if (key === 'ArrowLeft') {
    nextIndex = visibleIndexes[(position - 1 + visibleIndexes.length) % visibleIndexes.length]
  } else if (key === 'ArrowRight') {
    nextIndex = visibleIndexes[(position + 1) % visibleIndexes.length]
  } else if (key === 'Home') {
    nextIndex = visibleIndexes[0]
  } else if (key === 'End') {
    nextIndex = visibleIndexes[visibleIndexes.length - 1]
  }

  if (nextIndex === undefined) return false
  selectFilter(nextIndex)
  return true
}
