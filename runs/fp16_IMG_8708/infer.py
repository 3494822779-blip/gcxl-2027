from pathlib import Path
import json
import cv2
import numpy as np
import torch
import tensorrt as trt
root=Path('/home/nvidia/project/gcxl-2027')
out=root/'runs/fp16_IMG_8708'
img=cv2.imread('/home/nvidia/桌面/IMG_8708.JPG')
assert img is not None
h,w=img.shape[:2]; scale=min(640/w,640/h)
nw,nh=round(w*scale),round(h*scale)
left=(640-nw)//2; top=(640-nh)//2
canvas=np.full((640,640,3),114,np.uint8)
canvas[top:top+nh,left:left+nw]=cv2.resize(img,(nw,nh))
x=torch.from_numpy(np.ascontiguousarray(canvas[:,:,::-1].transpose(2,0,1))).unsqueeze(0).cuda().float()/255
logger=trt.Logger(trt.Logger.WARNING)
runtime=trt.Runtime(logger)
engine=runtime.deserialize_cuda_engine((root/'models/yolov8n_gcxl_aug/best_b1_gpu0_fp16.engine').read_bytes())
assert engine is not None
ctx=engine.create_execution_context(); tensors={}
for i in range(engine.num_io_tensors):
 name=engine.get_tensor_name(i); shape=tuple(engine.get_tensor_shape(name))
 dtype=torch.from_numpy(np.empty((),dtype=trt.nptype(engine.get_tensor_dtype(name)))).dtype
 tensors[name]=x.to(dtype).contiguous() if engine.get_tensor_mode(name)==trt.TensorIOMode.INPUT else torch.empty(shape,device='cuda',dtype=dtype)
 ctx.set_tensor_address(name,tensors[name].data_ptr())
stream=torch.cuda.current_stream(); assert ctx.execute_async_v3(stream.cuda_stream); stream.synchronize()
raw=tensors['output'].cpu().numpy()[0]
assert np.isfinite(raw).all()
rows=raw[raw[:,4]>=0.25]
kept=[]
for cls in np.unique(rows[:,5]):
 indices=np.where(rows[:,5]==cls)[0]
 boxes=rows[indices,:4].copy(); boxes[:,2:]-=boxes[:,:2]
 pick=cv2.dnn.NMSBoxes(boxes.tolist(),rows[indices,4].tolist(),0.25,0.45)
 for j in np.asarray(pick).reshape(-1): kept.append(rows[indices[j]])
results=[]
for row in sorted(kept,key=lambda r:float(r[4]),reverse=True):
 box=row[:4].copy(); box[[0,2]]=(box[[0,2]]-left)/scale; box[[1,3]]=(box[[1,3]]-top)/scale
 box[[0,2]]=np.clip(box[[0,2]],0,w); box[[1,3]]=np.clip(box[[1,3]],0,h)
 results.append(dict(class_id=int(row[5]),confidence=float(row[4]),xyxy=box.tolist()))
for threshold in (0.25,0.8):
 annotated=img.copy()
 for d in results:
  if d['confidence']<threshold: continue
  x1,y1,x2,y2=map(round,d['xyxy']); color=[(0,180,255),(0,200,0),(255,120,0),(180,0,220)][d['class_id']]
  cv2.rectangle(annotated,(x1,y1),(x2,y2),color,5)
  label=f"class_{d['class_id']} {d['confidence']:.3f}"
  cv2.putText(annotated,label,(x1,max(30,y1-12)),cv2.FONT_HERSHEY_SIMPLEX,1,color,3,cv2.LINE_AA)
 cv2.imwrite(str(out/f'result_conf_{threshold}.jpg'),annotated)
(out/'detections.json').write_text(json.dumps(dict(engine='best_b1_gpu0_fp16.engine',image_size=[w,h],confidence_threshold=0.25,nms_iou=0.45,detections=results),indent=2))
print(json.dumps(results,indent=2))
