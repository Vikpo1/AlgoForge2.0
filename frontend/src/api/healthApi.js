import http from './http'

export const getHealth = () => {
  return http.get('/api/health')
}