import http from './http'

export const getProblemLists = () => {
  return http.get('/api/problem-lists')
}

export const createProblemList = (payload) => {
  return http.post('/api/problem-lists', payload)
}

export const updateProblemList = (listId, payload) => {
  return http.patch(`/api/problem-lists/${listId}`, payload)
}

export const deleteProblemList = (listId) => {
  return http.delete(`/api/problem-lists/${listId}`)
}

export const getProblemsInList = (listId) => {
  return http.get(`/api/problem-lists/${listId}/problems`)
}

export const deleteProblemFromList = (listId, problemId) => {
  return http.delete(`/api/problem-lists/${listId}/problems/${problemId}`)
}
