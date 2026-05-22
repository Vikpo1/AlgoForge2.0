const feedbackTextMap = {
  FAILED: '完全不会',
  SOLVED_WITH_HINT: '看贴士才会',
  SOLVED_SLOWLY: '独立做出但较慢',
  SOLVED_FAST: '秒杀'
}

const statusTextMap = {
  FIRST_FIX: '赛后首补',
  COOLING: '冷却重刷',
  TRAINING: '穿插特训',
  MASTERED: '已掌握',
  ARCHIVED: '已归档'
}

export const getFeedbackText = (feedback) => {
  return feedbackTextMap[feedback] || feedback || '-'
}

export const getReviewStatusText = (status) => {
  return statusTextMap[status] || status || '-'
}
