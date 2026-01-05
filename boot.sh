# Comandos para criar um novo disk.tar com uma mensagem de boot customizada

# 1. Crie o arquivo 'boot_message.txt' com sua arte ASCII e mensagem.
#    Edite o conteúdo entre as aspas como desejar.

echo "escrevendo ascii art"
echo '

ooooo  oooo                       ooooooo    oooooooo8
  888  88 oooo  oooo oooo  oooo o888   888o 888
    888    888   888  888   888 888     888  888oooooo
    888    888   888  888   888 888o   o888         888
   o888o    888o88 8o  888o88 8o  88ooo88   o88oooo888


 (BOOTING YuuOS...)
' > boot_message.txt


echo "obtendo tamanho da mensagem e calculando preenchimento"
# 2. Obtenha o tamanho da mensagem e calcule o preenchimento necessário para alinhar ao próximo setor de 512 bytes.
MSG_SIZE=$(stat -f%z boot_message.txt)
PADDING_SIZE=$(( (512 - (MSG_SIZE % 512)) % 512 ))

echo "criando arquivo de preenchimento com zeros"
# 3. Crie um arquivo de preenchimento com zeros.
dd if=/dev/zero of=padding.bin bs=1 count=$PADDING_SIZE

echo "criando arquivo tar com sistema de arquivos"
# 4. Crie o arquivo tar com o sistema de arquivos, evitando metadados do macOS.
COPYFILE_DISABLE=1 tar --format=ustar -cvf fs_data.tar -C tmp/disk .

echo "combinando mensagem, preenchimento e sistema de arquivos para criar disk.tar"
# 5. Combine a mensagem, o preenchimento e o sistema de arquivos para criar o novo disk.tar.
cat boot_message.txt padding.bin fs_data.tar > disk.tar

echo "limpando arquivos temporarios"
# 6. Limpe os arquivos temporários.
rm boot_message.txt padding.bin fs_data.tar

echo "Novo disk.tar criado com sucesso!"
