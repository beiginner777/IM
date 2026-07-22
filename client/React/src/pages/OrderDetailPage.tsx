import { useEffect, useState } from 'react'
import { useParams, useNavigate } from 'react-router-dom'
import { Card, Button, Input, Modal, message, Spin, Descriptions, Tag } from 'antd'
import request from '../api/request'

export default function OrderDetailPage() {
  const { id } = useParams()
  const nav = useNavigate()
  const [order, setOrder] = useState<any>(null)
  const [pwdModal, setPwdModal] = useState(false)
  const [pwd, setPwd] = useState('')
  const [loading, setLoading] = useState(false)

  useEffect(() => { request.get('/order/'+id).then(r=>setOrder(r.data)).catch(()=>nav('/products')) }, [id])

  const doPay = async () => {
    setLoading(true)
    try {
      const res = await request.post('/order/'+id+'/pay', {password:pwd})
      if (res.data.success) { setOrder({...order,status:'paid'}); setPwdModal(false); message.success('支付成功！') }
      else message.error(res.data.message)
    } catch { message.error('支付失败') }
    finally { setLoading(false) }
  }

  const doCancel = () => {
    Modal.confirm({ title:'确认取消订单？', content:'订单将在30分钟后自动取消', onOk:async()=>{
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
          <Descriptions.Item label="时间">{order.time}</Descriptions.Item>
        </Descriptions>
        {order.status==='unpaid' && <div style={{display:'flex',gap:12,marginTop:24}}>
          <Button type="primary" block size="large" onClick={()=>setPwdModal(true)} style={{background:'linear-gradient(135deg,#667eea,#764ba2)',border:'none'}}>立即支付</Button>
          <Button block size="large" onClick={doCancel}>取消订单</Button>
        </div>}
      </Card>
      <Modal title="确认支付" open={pwdModal} onOk={doPay} onCancel={()=>setPwdModal(false)} confirmLoading={loading}>
        <Input.Password placeholder="请输入登录密码" onChange={e=>setPwd(e.target.value)} />
      </Modal>
    </div>
  )
}
