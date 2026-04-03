// === CƠ SỞ DỮ LIỆU TƯỚNG (58 tướng) - Đã cân bằng Võ/Văn + Quốc gia ===
const IMG_PATH = 'ảnh game/';
const CARD_BACK = IMG_PATH + 'matsau.jpg';
const KINGDOMS = {thuc:'Thục',nguy:'Ngụy',ngo:'Ngô',quan:'Quần'};
const KINGDOM_COLORS = {thuc:'#2ecc71',nguy:'#3498db',ngo:'#e74c3c',quan:'#f39c12'};
const cardDb = [
    // 5⭐
    { id:1, name:"Lữ Bố", stars:5, type:'vo', kingdom:'quan', atk:3500, def:2000, img:IMG_PATH+"lubo.jpg", desc:"Chiến thần vô địch thiên hạ, tay cầm Phương Thiên Họa Kích." },
    { id:2, name:"Quan Vũ", stars:5, type:'vo', kingdom:'thuc', atk:2900, def:1600, img:IMG_PATH+"quanvu.jpg", desc:"Võ thánh dùng Thanh Long Yển Nguyệt Đao, trung nghĩa vẹn toàn." },
    { id:3, name:"Triệu Vân", stars:5, type:'vo', kingdom:'thuc', atk:3000, def:1600, img:IMG_PATH+"trieuvan.jpg", desc:"Thất tiến thất xuất Trường Bản dốc, cứu ấu chúa A Đẩu." },
    { id:6, name:"Gia Cát Lượng", stars:5, type:'van', kingdom:'thuc', atk:1700, def:3100, img:IMG_PATH+"giacatluong.jpg", desc:"Ngọa Long tiên sinh, mưu lược hơn người. Lục xuất Kỳ Sơn." },
    // 4⭐
    { id:4, name:"Trương Phi", stars:4, type:'vo', kingdom:'thuc', atk:2700, def:1500, img:IMG_PATH+"trươngphi.jpg", desc:"Hổ tướng gầm vang cầu Trường Bản, một mình chặn vạn quân." },
    { id:5, name:"Điêu Thuyền", stars:4, type:'van', kingdom:'quan', atk:1500, def:2800, img:IMG_PATH+"dieuthuyen.jpg", desc:"Tứ đại mỹ nhân, dùng liên hoàn kế ly gián Đổng Trác và Lữ Bố." },
    { id:8, name:"Mã Siêu", stars:4, type:'vo', kingdom:'thuc', atk:3000, def:1600, img:IMG_PATH+"masieu.jpg", desc:"Cẩm Mã Siêu, uy chấn Tây Lương, đánh Tào Tháo cắt râu bỏ áo." },
    { id:9, name:"Hoàng Trung", stars:4, type:'vo', kingdom:'thuc', atk:2800, def:1600, img:IMG_PATH+"hoangtrung.jpg", desc:"Lão tướng bách phát bách trúng, chém Hạ Hầu Uyên tại Định Quân Sơn." },
    { id:11, name:"Tôn Sách", stars:4, type:'vo', kingdom:'ngo', atk:2800, def:1600, img:IMG_PATH+"TonSach.jpg", desc:"Tiểu Bá Vương, bình định Giang Đông, dũng mãnh vô song." },
    { id:16, name:"Tư Mã Ý", stars:4, type:'van', kingdom:'nguy', atk:1700, def:3100, img:IMG_PATH+"tumay.jpg", desc:"Lang Cố chi tâm, mưu sĩ nhẫn nại, đặt nền móng cho nhà Tấn." },
    { id:17, name:"Tào Tháo", stars:4, type:'van', kingdom:'nguy', atk:1700, def:3100, img:IMG_PATH+"Taothao.jpg", desc:"Gian hùng thời loạn, mưu lược hơn người, thống nhất phương Bắc." },
    { id:18, name:"Chu Du", stars:4, type:'van', kingdom:'ngo', atk:1600, def:2900, img:IMG_PATH+"ChuDu.jpg", desc:"Đại đô đốc Đông Ngô, hỏa thiêu Xích Bích, tài kiêm văn võ." },
    { id:20, name:"Điển Vi", stars:4, type:'vo', kingdom:'nguy', atk:3100, def:1700, img:IMG_PATH+"DienVi.jpg", desc:"Hộ vệ trung thành của Tào Tháo, một mình chặn vạn quân." },
    { id:22, name:"Hứa Chử", stars:4, type:'vo', kingdom:'nguy', atk:2900, def:1600, img:IMG_PATH+"HuaChu.jpg", desc:"Hổ Sĩ, lực sĩ vô song, lõa bỏ áo đấu Mã Siêu." },
    { id:25, name:"Trương Liêu", stars:4, type:'vo', kingdom:'nguy', atk:2900, def:1600, img:IMG_PATH+"TruongLieu.jpg", desc:"Uy chấn Hợp Phì, 800 quân phá 10 vạn." },
    { id:46, name:"Thái Sử Từ", stars:4, type:'vo', kingdom:'ngo', atk:3000, def:1600, img:IMG_PATH+"thaisutu.jpg", desc:"Mãnh tướng Giang Đông, đấu tay đôi với Tôn Sách." },
    // 3⭐
    { id:7, name:"Lưu Bị", stars:3, type:'van', kingdom:'thuc', atk:1400, def:2600, img:IMG_PATH+"luubi.jpg", desc:"Huyền Đức nhân nghĩa, sáng lập nhà Thục Hán." },
    { id:10, name:"Tôn Quyền", stars:3, type:'van', kingdom:'ngo', atk:1500, def:2900, img:IMG_PATH+"tonquyen.jpeg", desc:"Bích Nhãn Nhi, chúa Đông Ngô." },
    { id:12, name:"Hạ Hầu Đôn", stars:3, type:'vo', kingdom:'nguy', atk:2800, def:1600, img:IMG_PATH+"HaHauDon.jpg", desc:"Độc nhãn tướng quân, nuốt con ngươi giữa trận." },
    { id:13, name:"Viên Thiệu", stars:3, type:'van', kingdom:'quan', atk:1500, def:2900, img:IMG_PATH+"VienThieu.jpg", desc:"Đại chư hầu phương Bắc, bại trận Quan Độ." },
    { id:14, name:"Bàng Thống", stars:3, type:'van', kingdom:'thuc', atk:1600, def:2900, img:IMG_PATH+"bangthong.jpg", desc:"Phượng Sồ tiên sinh, tài trí sánh ngang Gia Cát Lượng." },
    { id:19, name:"Cam Ninh", stars:3, type:'vo', kingdom:'ngo', atk:2600, def:1400, img:IMG_PATH+"CamNinh.jpg", desc:"Kình Châu Cam Hưng Bá, dũng mãnh tuyệt luân." },
    { id:21, name:"Đổng Trác", stars:3, type:'van', kingdom:'quan', atk:1500, def:2500, img:IMG_PATH+"DongTrac.jpg", desc:"Bạo chúa lộng quyền, khống chế thiên tử." },
    { id:23, name:"Ngụy Diên", stars:3, type:'vo', kingdom:'thuc', atk:2700, def:1500, img:IMG_PATH+"NguyDien.jpg", desc:"Mãnh tướng phản chu, bị chém theo kế Khổng Minh." },
    { id:24, name:"Quách Gia", stars:3, type:'van', kingdom:'nguy', atk:1500, def:2700, img:IMG_PATH+"QuachGia.jpg", desc:"Mưu sĩ tài ba yểu mệnh của Tào Tháo." },
    { id:29, name:"Giả Hủ", stars:3, type:'van', kingdom:'nguy', atk:1500, def:2700, img:IMG_PATH+"giahu.jpg", desc:"Độc sĩ Giả Hủ, mưu kế hiểm độc." },
    { id:32, name:"Khương Duy", stars:3, type:'vo', kingdom:'thuc', atk:2800, def:1700, img:IMG_PATH+"khuongduy.jpg", desc:"Truyền nhân của Gia Cát Lượng, 9 lần Bắc phạt." },
    { id:34, name:"Lã Mông", stars:3, type:'vo', kingdom:'ngo', atk:2700, def:1500, img:IMG_PATH+"lamong.jpg", desc:"Bạch y qua sông đánh Kinh Châu, hạ Quan Vũ." },
    { id:37, name:"Lục Tốn", stars:3, type:'van', kingdom:'ngo', atk:1600, def:2900, img:IMG_PATH+"lucton.jpg", desc:"Đại đô đốc Đông Ngô, hỏa thiêu doanh trại Di Lăng." },
    { id:42, name:"Nhan Lương", stars:3, type:'vo', kingdom:'quan', atk:2700, def:1400, img:IMG_PATH+"nhanluong.jpg", desc:"Mãnh tướng Viên Thiệu, bị Quan Vũ chém tại Bạch Mã." },
    { id:48, name:"Tôn Kiên", stars:3, type:'vo', kingdom:'ngo', atk:2800, def:1500, img:IMG_PATH+"tonkien.jpg", desc:"Hổ tướng Giang Đông, cha của Tôn Sách và Tôn Quyền." },
    { id:52, name:"Trương Cáp", stars:3, type:'vo', kingdom:'nguy', atk:2700, def:1600, img:IMG_PATH+"truongcap.jpg", desc:"Ngũ Tử Lương Tướng, giỏi xoay chuyển tình thế." },
    { id:55, name:"Tuân Úc", stars:3, type:'van', kingdom:'nguy', atk:1500, def:2700, img:IMG_PATH+"tuanuc.jpg", desc:"Vương Tá chi tài, mưu sĩ hàng đầu Tào Tháo." },
    { id:57, name:"Văn Xú", stars:3, type:'vo', kingdom:'quan', atk:2600, def:1400, img:IMG_PATH+"vanxu.jpg", desc:"Mãnh tướng Viên Thiệu, dũng cảm xung phong." },
    // 2⭐
    { id:15, name:"Hoa Hùng", stars:2, type:'vo', kingdom:'quan', atk:2600, def:1400, img:IMG_PATH+"hoahung.jpg", desc:"Mãnh tướng Đổng Trác, bị Quan Vũ chém khi rượu còn nóng." },
    { id:27, name:"Chu Thái", stars:2, type:'vo', kingdom:'ngo', atk:2600, def:1500, img:IMG_PATH+"chuthai.jpg", desc:"Mãnh tướng Đông Ngô, cứu Tôn Quyền thoát hiểm." },
    { id:30, name:"Hoàng Cái", stars:2, type:'vo', kingdom:'ngo', atk:2500, def:1500, img:IMG_PATH+"hoangcai.jpg", desc:"Lão tướng Đông Ngô, dùng khổ nhục kế tại Xích Bích." },
    { id:33, name:"Kỳ Linh", stars:2, type:'vo', kingdom:'quan', atk:2600, def:1400, img:IMG_PATH+"kylinh.jpg", desc:"Đại tướng Viên Thuật, cầm Tam Tiêm Lưỡng Nhật Đao." },
    { id:35, name:"Liêu Hóa", stars:2, type:'vo', kingdom:'thuc', atk:2400, def:1300, img:IMG_PATH+"lieuhoa.jpg", desc:"Lão tướng Thục Hán, trung thành son sắt." },
    { id:36, name:"Lỗ Túc", stars:2, type:'van', kingdom:'ngo', atk:1400, def:2600, img:IMG_PATH+"lotuc.jpg", desc:"Mưu sĩ Đông Ngô, chủ trương liên minh Tôn-Lưu." },
    { id:39, name:"Mã Đại", stars:2, type:'vo', kingdom:'thuc', atk:2500, def:1400, img:IMG_PATH+"madai.jpg", desc:"Em họ Mã Siêu, chém Ngụy Diên theo kế Gia Cát Lượng." },
    { id:41, name:"Nhạc Tiến", stars:2, type:'vo', kingdom:'nguy', atk:2700, def:1500, img:IMG_PATH+"nhactien.jpg", desc:"Ngũ Tử Lương Tướng, trấn thủ Hợp Phì." },
    { id:43, name:"Pháp Chính", stars:2, type:'van', kingdom:'thuc', atk:1400, def:2700, img:IMG_PATH+"phapchinh.jpg", desc:"Mưu sĩ Lưu Bị, bày kế chiếm Ích Châu." },
    { id:44, name:"Quan Bình", stars:2, type:'vo', kingdom:'thuc', atk:2500, def:1400, img:IMG_PATH+"quanbinh.jpg", desc:"Con trai Quan Vũ, cùng cha tử trận Mạch Thành." },
    { id:45, name:"Tào Nhân", stars:2, type:'vo', kingdom:'nguy', atk:2500, def:1700, img:IMG_PATH+"taonhan.jpg", desc:"Em họ Tào Tháo, trấn thủ Phàn Thành." },
    { id:49, name:"Trình Dục", stars:2, type:'van', kingdom:'nguy', atk:1400, def:2600, img:IMG_PATH+"trinhduc.jpg", desc:"Mưu sĩ Tào Tháo, tính toán sâu xa." },
    { id:50, name:"Trình Phổ", stars:2, type:'vo', kingdom:'ngo', atk:2600, def:1400, img:IMG_PATH+"trinhpho.jpg", desc:"Lão tướng Đông Ngô, phục vụ ba đời họ Tôn." },
    { id:51, name:"Trương Bao", stars:2, type:'vo', kingdom:'thuc', atk:2500, def:1400, img:IMG_PATH+"truongbao.jpg", desc:"Con Trương Phi, theo Gia Cát Lượng Bắc phạt." },
    { id:53, name:"Trương Giác", stars:2, type:'van', kingdom:'quan', atk:1400, def:2700, img:IMG_PATH+"truonggiac.jpg", desc:"Giáo chủ Thái Bình Đạo, tự xưng Thiên Công Tướng Quân." },
    { id:56, name:"Từ Hoảng", stars:2, type:'vo', kingdom:'nguy', atk:2700, def:1500, img:IMG_PATH+"tuhoang.jpg", desc:"Ngũ Tử Lương Tướng, giải vây Phàn Thành." },
    { id:58, name:"Vu Cấm", stars:2, type:'vo', kingdom:'nguy', atk:2600, def:1400, img:IMG_PATH+"vucam.jpg", desc:"Ngũ Tử Lương Tướng, đầu hàng nhục nhã." },
    // 1⭐
    { id:26, name:"Châu Thương", stars:1, type:'vo', kingdom:'thuc', atk:2300, def:1300, img:IMG_PATH+"chauthuong.jpg", desc:"Hộ vệ trung thành của Quan Vũ." },
    { id:28, name:"Chu Tuấn", stars:1, type:'vo', kingdom:'quan', atk:2400, def:1500, img:IMG_PATH+"chutuan.jpg", desc:"Danh tướng nhà Hán, dẹp loạn Khăn Vàng." },
    { id:31, name:"Hoàng Phủ Tung", stars:1, type:'vo', kingdom:'quan', atk:2400, def:1500, img:IMG_PATH+"hoangphutung.jpg", desc:"Danh tướng nhà Hán, dẹp tặc Khăn Vàng." },
    { id:38, name:"Lư Thực", stars:1, type:'van', kingdom:'quan', atk:1300, def:2500, img:IMG_PATH+"luthuc.jpg", desc:"Thầy của Lưu Bị, hải nội đại nho." },
    { id:40, name:"My Trúc", stars:1, type:'van', kingdom:'thuc', atk:1200, def:2300, img:IMG_PATH+"mychuc.jpg", desc:"Phú thương giàu có, tài trợ Lưu Bị dựng nghiệp." },
    { id:47, name:"Tôn Càn", stars:1, type:'van', kingdom:'thuc', atk:1200, def:2200, img:IMG_PATH+"toncan.jpg", desc:"Tùng sĩ trung thành của Lưu Bị, ngoại giao giỏi." },
    { id:54, name:"Trương Lương", stars:1, type:'vo', kingdom:'quan', atk:2400, def:1500, img:IMG_PATH+"truongluong.jpg", desc:"Nhân Công Tướng Quân, em Trương Giác, khởi loạn Khăn Vàng." }
];

// === DỮ LIỆU CỐT TRUYỆN (STORY MODE) ===
const storyData = [
    {
        id: 'story_1',
        title: 'Chương 1: Khởi Nghĩa Khăn Vàng',
        description: 'Ba anh em họ Trương nổi dậy, trời đất điên đảo. Hãy dẹp loạn Khăn Vàng!',
        enemyName: 'Trương Giác',
        difficulty: 'easy',
        enemyCards: [53, 54, 28, 31, 28, 31, 53, 54], // IDs để build bộ bài địch
        playerCards: [7, 2, 4, 40, 47, 44, 51, 26, 35, 38], // Lưu Bị, Quan Vũ, Trương Phi, Lư Thực...
        rewardCards: [53, 54, 28, 31], // Mở khóa: Trương Giác, Trương Lương, Chu Tuấn, Hoàng Phủ Tung
        goldReward: 500
    },
    {
        id: 'story_2',
        title: 'Chương 2: Ác Chiến Hổ Lao Quan',
        description: 'Đổng Trác lộng quyền, Lữ Bố vô địch thiên hạ chặn ở Hổ Lao Quan. Hãy cẩn thận!',
        enemyName: 'Đổng Trác & Lữ Bố',
        difficulty: 'medium',
        enemyCards: [1, 21, 15, 5, 21, 15, 1, 5], // Lữ Bố, Đổng Trác, Hoa Hùng, Điêu Thuyền
        playerCards: [17, 20, 22, 13, 33, 42, 57, 48, 41, 12], // Tào Tháo, Viên Thiệu, Tôn Kiên, Hạ Hầu Đôn...
        rewardCards: [1, 21, 15, 5], 
        goldReward: 1000
    },
    {
        id: 'story_3',
        title: 'Chương 3: Quần Hùng Cát Cứ',
        description: 'Đổng Trác đền tội, nhưng các chư hầu bắt đầu cấu xé lẫn nhau. Đối mặt Viên Thiệu!',
        enemyName: 'Viên Thiệu',
        difficulty: 'medium',
        enemyCards: [13, 42, 57, 33, 13, 42, 57, 33], // Viên Thiệu, Nhan Lương, Văn Xú, Kỳ Linh
        playerCards: [17, 20, 22, 12, 41, 52, 24, 55, 45, 56], // Tào Tháo và các tướng Ngụy
        rewardCards: [13, 42, 57, 33],
        goldReward: 1500
    },
    {
        id: 'story_4',
        title: 'Chương 4: Đại Chiến Xích Bích',
        description: 'Tào Tháo dẫn trăm vạn đại quân xuống miền Nam. Liên minh Tôn - Lưu lập mưu hỏa thiêu Xích Bích!',
        enemyName: 'Tào Tháo',
        difficulty: 'hard',
        enemyCards: [17, 24, 20, 22, 25, 12, 41, 17, 20, 25], // Tào Tháo, Điển Vi, Hứa Chử, v.v..
        playerCards: [18, 50, 48, 11, 10, 46, 30, 36, 19, 6], // Châu Du, Tôn Quyền, Lỗ Túc, Tôn Sách, Hoàng Cái...
        rewardCards: [17, 24, 20, 22],
        goldReward: 2500
    },
    {
        id: 'story_5',
        title: 'Chương 5: Di Lăng Hỏa Công',
        description: 'Lưu Bị báo thù cho Quan Vũ, mang đại quân Thục phạt Ngô. Hãy phòng thủ và phản công!',
        enemyName: 'Lưu Bị',
        difficulty: 'hard',
        enemyCards: [7, 6, 2, 3, 4, 8, 9, 7, 2, 4], // Thục Hán
        playerCards: [37, 34, 11, 46, 19, 27, 30, 10, 18, 50], // Lục Tốn, Lã Mông, và các Tướng Ngô
        rewardCards: [7, 2, 4, 9], // Nhận Lưu Bị, Quan Vũ, Trương Phi, Hoàng Trung (Gia cát lượng và Triệu vân có thể may mắn trong Gacha)
        goldReward: 3500
    }
];
