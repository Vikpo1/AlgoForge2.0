import http from './http'

// 获取下一道复习题：mode=weighted 表示按题单权重调度，mode=random 表示纯随机
export const getNextReviewProblem = ({ mode = 'weighted', listId = null } = {}) => {
  const params = { mode }

  if (listId) {
    params.listId = listId
  }

  return http.get('/api/review/next', { params })
}
