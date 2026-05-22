import os
from PIL import Image

# Đường dẫn tới thư mục ảnh của bạn
folder_path = "anh_game" 

print("Bắt đầu chuyển đổi ảnh...")

# Quét qua tất cả các file trong thư mục
for filename in os.listdir(folder_path):
    # Tìm các file có đuôi jpg, jpeg, webp
    if filename.lower().endswith((".jpg", ".jpeg", ".webp")):
        img_path = os.path.join(folder_path, filename)
        
        try:
            # Mở ảnh bằng Pillow
            with Image.open(img_path) as img:
                # Chuyển sang hệ màu RGB (để fix lỗi các ảnh có kênh alpha/trong suốt bị lỗi khi convert)
                rgb_im = img.convert('RGB')
                
                # Tạo tên file mới với đuôi .png
                new_filename = os.path.splitext(filename)[0] + ".png"
                new_filepath = os.path.join(folder_path, new_filename)
                
                # Lưu file mới dưới chuẩn PNG
                rgb_im.save(new_filepath, "PNG")
                print(f"✅ Đã chuyển: {filename} -> {new_filename}")
                
            # Xóa dòng '#' ở dưới nếu bạn muốn script TỰ ĐỘNG XÓA file .jpg cũ đi cho gọn thư mục
            # os.remove(img_path) 
            
        except Exception as e:
            print(f"❌ Lỗi với file {filename}: {e}")

print("🎉 Đã hoàn thành convert toàn bộ ảnh sang chuẩn PNG!")