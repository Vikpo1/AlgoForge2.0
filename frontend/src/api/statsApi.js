import http from './http'

export const getDailyActivity = (days = 365) => {
  return http.get('/api/stats/daily-activity', {
    params: { days }
  })
}
