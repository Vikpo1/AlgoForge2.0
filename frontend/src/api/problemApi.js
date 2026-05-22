import http from './http'

export const importProblem = (payload) => {
  return http.post('/api/problems/import', payload)
}

export const updateProblemWeight = (problemId, problemUserWeight) => {
  return http.patch(`/api/problems/${problemId}/weight`, {
    problemUserWeight
  })
}

export const getProblemDetail = (problemId) => {
  return http.get(`/api/problems/${problemId}/detail`)
}
