import os

# Chỉ định đúng thư mục chứa ảnh
folder_path = "anh_game"

print("Bắt đầu dọn dẹp các file ảnh cũ...")
deleted_count = 0

# Quét tất cả các file trong thư mục anh_game
for filename in os.listdir(folder_path):
    # Nếu file có đuôi là jpg, jpeg hoặc webp thì tiến hành xóa
    if filename.lower().endswith((".jpg", ".jpeg", ".webp")):
        file_path = os.path.join(folder_path, filename)
        
        try:
            os.remove(file_path) # Lệnh xóa file
            print(f"🗑️ Đã xóa: {filename}")
            deleted_count += 1
        except Exception as e:
            print(f"❌ Lỗi khi xóa {filename}: {e}")

print(f"🎉 Hoàn tất! Đã dọn dẹp sạch sẽ {deleted_count} file ảnh cũ.")