import axios from 'axios'
import { clearAuthSession, getAuthToken } from '../utils/auth'

const http = axios.create({
  baseURL: import.meta.env.VITE_API_BASE_URL || '',
  timeout: 5000,
  headers: {
    'Content-Type': 'application/json'
  }
})

http.interceptors.request.use(config => {
  const token = getAuthToken()
  if (token) {
    config.headers.Authorization = `Bearer ${token}`
  }
  return config
})

http.interceptors.response.use(
  response => {
    return response
  },
  error => {
    if (error.response?.status === 401) {
      clearAuthSession()
      if (window.location.pathname !== '/login') {
        window.location.href = '/login'
      }
    }
    return Promise.reject(error)
  }
)

export default http
