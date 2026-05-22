import http from './http'

// 提交某道题的复习反馈
export const submitReviewFeedback = (problemId, feedback, durationSeconds = 0) => {
  return http.post(`/api/review/${problemId}/feedback`, {
    feedback,
    durationSeconds
  })
}