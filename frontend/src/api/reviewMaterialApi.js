import http from './http'

export const getReviewMaterial = (problemId) => {
  return http.get(`/api/problems/${problemId}/review-material`)
}

export const updateReviewMaterial = (problemId, payload) => {
  return http.patch(`/api/problems/${problemId}/review-material`, payload)
}
