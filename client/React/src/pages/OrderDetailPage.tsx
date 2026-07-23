import { useEffect, useState } from 'react'
import { useParams, useNavigate } from 'react-router-dom'
import { Card, Button, Input, Modal, message, Spin, Descriptions, Tag } from 'antd'
import { ClockCircleOutlined } from '@ant-design/icons'
import request from '../api/request'

function fmtTime(s: number) { const m=Math.floor(s/60), sec=s%60; return `${m}:${sec.toString().padStart(2,'0')}` }

export default function OrderDetailPage() {
  const { id } = useParams()
  const nav = useNavigate()
  const [order, setOrder] = useState<any>(null)
  const [pwdModal, setPwdModal] = useState(false)
  const [pwd, setPwd] = useState('')
  const [loading, setLoading] = useState(false)
  const [remain, setRemain] = useState(1800)

  const loadOrder = () => request.get('/order/'+id).then(r=>{
    setOrder(r.data)
    if (r.data.status==='unpaid') setRemain(1800)
  }).catch(()=>nav('/products'))

  useEffect(() => { loadOrder() }, [id])

  // Countdown
  useEffect(() => {
    if (!order || order.status!=='unpaid') return
    const t = setInterval(() => {
      setRemain(r => { if (r<=1) { loadOrder(); return 0 } return r-1 })
    }, 1000)
    return () => clearInterval(t)
  }, [order])

  const expired = remain <= 0

  const doPay = async () => {
    setLoading(true)
    try {
      const res = await request.post('/order/'+id+'/pay', {password:pwd})
      if (res.data.success) { setOrder({...order,status:'paid'}); setPwdModal(false); message.success('支付成功！') }
      else { setPwdModal(false); setPwd(''); message.error(res.data.message) }
    } catch { setPwdModal(false); setPwd('') }
    finally { setLoading(false) }
  }

  const doCancel = () => {
    Modal.confirm({ title:'确认取消订单？', content:'订单将被取消', onOk:async()=>{
      try { await request.post('/order/'+id+'/cancel'); setOrder({...order,status:'cancelled'}); message.info('订单已取消') }
      catch { message.error('取消失败') }
    }})
  }

  if (!order) return <div style={{minHeight:'100vh',background:'#0a0a1a',display:'flex',justifyContent:'center',alignItems:'center'}}><Spin size="large"/></div>

  const statusMap: any = { unpaid:{color:'orange',text:'待支付'}, paid:{color:'green',text:'已支付'}, cancelled:{color:'red',text:'已取消'} }
  const st = statusMap[order.status] || {color:'default',text:order.status}

  return (
    <div style={{minHeight:'100vh',background:'#0a0a1a',padding:40,display:'flex',justifyContent:'center'}}>
      <Card style={{width:500,background:'rgba(255,255,255,0.04)',border:'1px solid rgba(255,255,255,0.08)',borderRadius:16}}>
        <h2 style={{color:'#fff',textAlign:'center',marginBottom:24}}>订单详情</h2>
        <Descriptions column={1} labelStyle={{color:'#aaa'}} contentStyle={{color:'#fff'}} bordered={false}>
          <Descriptions.Item label="订单号">#{order.id}</Descriptions.Item>
          <Descriptions.Item label="商品">{order.productName}</Descriptions.Item>
          <Descriptions.Item label="金额"><span style={{color:'#ffd700',fontSize:20,fontWeight:'bold'}}>¥{order.price}</span></Descriptions.Item>
          <Descriptions.Item label="状态"><Tag color={st.color}>{st.text}</Tag></Descriptions.Item>
          {order.status==='unpaid' && <Descriptions.Item label="剩余时间">
            <span style={{color:expired?'red':'#ffd700'}}><ClockCircleOutlined /> {expired?'已超时':fmtTime(remain)}</span>
          </Descriptions.Item>}
          <Descriptions.Item label="时间">{order.time}</Descriptions.Item>
        </Descriptions>
        {order.status==='unpaid' && !expired && <div style={{display:'flex',gap:12,marginTop:24}}>
          <Button type="primary" block size="large" onClick={()=>setPwdModal(true)} style={{background:'linear-gradient(135deg,#667eea,#764ba2)',border:'none'}}>立即支付</Button>
          <Button block size="large" onClick={doCancel}>取消订单</Button>
        </div>}
        {order.status==='unpaid' && expired && <div style={{textAlign:'center',marginTop:24,color:'red'}}>订单已超时，无法支付</div>}
      </Card>
      <Modal title="确认支付" open={pwdModal} onOk={doPay} onCancel={()=>{setPwdModal(false);setPwd('')}} confirmLoading={loading}>
        <Input.Password placeholder="请输入登录密码" onChange={e=>setPwd(e.target.value)} />
      </Modal>
    </div>
  )
}
