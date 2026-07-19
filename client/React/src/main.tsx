import ReactDOM from 'react-dom/client'
import { BrowserRouter } from 'react-router-dom'
import { ConfigProvider, theme } from 'antd'
import App from './App'

ReactDOM.createRoot(document.getElementById('root')!).render(
  <ConfigProvider theme={{ algorithm: theme.darkAlgorithm, token: { colorPrimary: '#667eea' } }}>
    <BrowserRouter>
      <App />
    </BrowserRouter>
  </ConfigProvider>
)
