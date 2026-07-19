import { useNavigate } from 'react-router-dom'
import { Button, Typography } from 'antd'
import { useAuthStore } from '../store/auth'

const features = [
  { icon: '⚛️', title: 'Redis Lua 原子扣减', desc: '单线程 Lua 脚本保证库存扣减原子性，杜绝超卖' },
  { icon: '📨', title: '消息队列异步削峰', desc: 'RabbitMQ 将订单写入、通知、排行榜异步并行处理' },
  { icon: '🖥️', title: 'C++ Beast 高性能', desc: 'Boost Beast HTTP 服务，微秒级请求处理延迟' },
  { icon: '📊', title: '实时排行榜', desc: 'Redis ZSET 毫秒级更新抢购排名，数据实时可见' },
]

export default function HomePage() {
  const nav = useNavigate()
  const { isLoggedIn } = useAuthStore()

  return (
    <div style={{ minHeight: 'calc(100vh - 56px)', background: 'linear-gradient(160deg, #0f0c29, #1a1a3e 30%, #24243e 60%, #1a1a3e)', display: 'flex', flexDirection: 'column', alignItems: 'center', padding: '80px 24px 60px', position: 'relative', overflow: 'hidden' }}>
      {/* glow */}
      <div style={{ position: 'fixed', width: 700, height: 700, borderRadius: '50%', filter: 'blur(140px)', opacity: .1, background: '#667eea', top: -250, right: -200, pointerEvents: 'none' }} />
      <div style={{ position: 'fixed', width: 500, height: 500, borderRadius: '50%', filter: 'blur(140px)', opacity: .1, background: '#f093fb', bottom: -200, left: -150, pointerEvents: 'none' }} />

      {/* hero */}
      <div style={{ textAlign: 'center', maxWidth: 780, zIndex: 1 }}>
        <span style={{ display: 'inline-block', padding: '6px 18px', borderRadius: 20, border: '1px solid rgba(102,126,234,0.35)', fontSize: 13, color: '#667eea', letterSpacing: 2, marginBottom: 28 }}>🚀 高性能秒杀引擎</span>
        <h1 style={{ fontSize: 54, fontWeight: 800, color: '#fff', lineHeight: 1.3, marginBottom: 20 }}>
          限时<span style={{ background: 'linear-gradient(135deg,#667eea 20%,#f093fb 80%)', WebkitBackgroundClip: 'text', WebkitTextFillColor: 'transparent' }}> 秒杀 </span>· 手速决胜
        </h1>
        <Typography.Paragraph style={{ fontSize: 17, color: 'rgba(255,255,255,0.5)', marginBottom: 40 }}>
          基于 C++ Beast 构建的高并发秒杀平台<br />
          Redis Lua 原子脚本防超卖 · RabbitMQ 异步削峰 · 毫秒级抢购体验
        </Typography.Paragraph>
        <div style={{ display: 'flex', gap: 16, justifyContent: 'center' }}>
          {isLoggedIn ? (
            <Button type="primary" size="large" onClick={() => nav('/products')} style={{ background: 'linear-gradient(135deg,#667eea,#764ba2)', border: 'none', fontWeight: 700, boxShadow: '0 4px 20px rgba(102,126,234,0.4)' }}>进入商城 →</Button>
          ) : (
            <>
              <Button type="primary" size="large" onClick={() => nav('/login')} style={{ background: 'linear-gradient(135deg,#667eea,#764ba2)', border: 'none', fontWeight: 700, boxShadow: '0 4px 20px rgba(102,126,234,0.4)' }}>立即抢购 →</Button>
              <Button size="large" ghost onClick={() => nav('/register')}>创建账号</Button>
            </>
          )}
        </div>
      </div>

      {/* features */}
      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4,1fr)', gap: 24, maxWidth: 960, width: '100%', marginTop: 72, zIndex: 1 }}>
        {features.map((f, i) => (
          <div key={i} style={{ background: 'rgba(255,255,255,0.03)', border: '1px solid rgba(255,255,255,0.06)', borderRadius: 18, padding: '32px 24px', textAlign: 'center', transition: 'all .3s', cursor: 'default' }}
            onMouseEnter={e => { (e.currentTarget as HTMLElement).style.transform = 'translateY(-4px)'; (e.currentTarget as HTMLElement).style.borderColor = 'rgba(102,126,234,0.25)' }}
            onMouseLeave={e => { (e.currentTarget as HTMLElement).style.transform = ''; (e.currentTarget as HTMLElement).style.borderColor = '' }}>
            <div style={{ fontSize: 36, marginBottom: 16 }}>{f.icon}</div>
            <div style={{ fontSize: 15, fontWeight: 700, color: '#e0e0e0', marginBottom: 8 }}>{f.title}</div>
            <div style={{ fontSize: 13, color: 'rgba(255,255,255,0.4)', lineHeight: 1.6 }}>{f.desc}</div>
          </div>
        ))}
      </div>

      <footer style={{ textAlign: 'center', padding: 32, marginTop: 'auto', fontSize: 13, color: 'rgba(255,255,255,0.18)', letterSpacing: 1, zIndex: 1 }}>
        Built with C++ · Boost Beast · Redis · RabbitMQ &nbsp;|&nbsp; IM Flash Sale Platform
      </footer>
    </div>
  )
}
