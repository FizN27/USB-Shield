# USB-Shield

USB Shield adalah sebuah aplikasi untuk melakukan management USB yang terhubung ke suatu komputer.

Aplikasi ini terbagi menjadi dua, GUI yang dapat digunakan untuk mengakses fitur-fitur USB Shield dan helper yang berjalan di background untuk melakukan deteksi terhadap USB yang dihubungkan ke sistem.

Untuk melakukan instalasi, silahkan run file "install_UsbShield.bat" as Administrator. 

Script ini akan melakukan hal-hal sebagai berikut:
1. Menambahkan registry "HKLM:\SOFTWARE\USBShield" untuk menyimpan data-data yang akan digunakan
2. Mengcopy file aplikasi ke directory "C:\Program Files\USB Shield"
3. Membuat task scheduler di path "\UBSShield" dengan nama "USB Shield Helper" yang akan berjalan setiap Startup Komputer berlangsung
4. Menjalankan helper setelah instalasi dilakukan

Untuk melakukan uninstall, silahkan run file "uninstall_USBShield.bat" as Adminstrator.

Fitur-fitur yang terdapat di Aplikasi GUI:
1. Main Menu
Tampilan utama dari aplikasi.
2. User Settings
Untuk melakukan konfigurasi terhadap aplikasi USB Shield. Perlu diisi untuk menerima email notifikasi.
3. Search Bar
Untuk melakukan pencarian data di tabel. Masing-masing tabel memiliki tombolnya sendiri.
4. Refresh Data
Untuk melakukan pembaharuan data sesuai dengan registry terbaru.
5. Name or Rename Data
Untuk memberi atau mengubah label USB yang ada di tabel(Label ini hanya berlaku di aplikasi USB Shield saja).
6. Move to Blacklist or Whitelist
Untuk memindahkan data dari Blacklist ke Whitelist atau sebaliknya.
7. Delete from Blacklist or Whitelist
Untuk menghapus data dari Blacklist atau Whitelist.
