checked xp sp2���pci.sys������

��ʵ�����Ū����
ֻ��pci bridge����Ϊ��Щ����
�����Ҿ���׼�����ĵ�ɶ��
����Ҷ�д�Ĳ����
�ŷ���΢���Լ�����վ������Щ�ĵ�
���ǾͰѴ���������������

===================================================
΢����ĵ�������
�����ǹ���pci bridge��
http://www.microsoft.com/whdc/system/bus/PCI/pcibridge-cardbus.mspx

�����ǹ���subtractive decode��
http://download.microsoft.com/download/5/b/5/5b5bec17-ea71-4653-9539-204a672f11cf/PCIbridge-subtr.doc
===================================================

pci���������������Ͻ�����Ҫʵ��4������
1.ö�������ϵ��豸..�����Ҷ�֪����..
2.pci�ĵ�Դ����,��Ҳ��pci�淶����Ķ���ûʲô�ر��
3.pci�������豸��resource�ٲ�....���Ҫռ�ܴ����..������Դ�ٲÿ���˵��windows��pnp�ܹ�����һ������Ҫ����ɲ���.
   �������������һ��np-complete������.���Կ���windows���ں˲������np-complete����Ȥʵ��
4.pci���ṩ�����ɸ�interface,

1.ö���豸��������.����һ��Ƕ�׵�ѭ��ȥ���Զ�ȡconfig space.��ȡ�ɹ��˾��ҵ���һ�����豸,Ȼ��ʹ�����
2.��Դ����͸�������..ддconfig space�ͳ�
3.��Դ�ٲÿ������ܸ�����ʵ����.�������ǹ̶��Ĵ���ʽ...
   ��λ��ͷ��windows 2000Դ�����ͬѧ���Կ���ntos\arbĿ¼.������aribter��ʵ��
   Ȼ����io\pnpres.c�����pnp����Դ�����ʵ��
   ��Ȼ����windowsʹ�õ��㷨��2000�кܶ಻ͬ��.����ԭ�������һ����
   �����Ͻ�...���np-complete��������Ե�ͬ��һ����������..
4.���ɸ�interface.��ʵÿ�����ܼ�

ʵ����ֱ��2003.windows��pci������������������
���Խ�..windows��pci����������������һ����ȷʵ�ֵ�bios
����windows��pci����������hotplug��֧�ַǳ��ǳ�������

΢������˵vista��pci��������������������������
������û����vista�İ汾.����Ͳ������...

����...�����������������..����2003/xp�Ļ����µ���
��Ҫ��vista����ȥ����...

����...pci.sys�Ǹ�boot driver.����Ҫһ��checked���ntldr����ʹ��kdfiles���滻Ŀ��ϵͳ�����boot driver
windows�Լ���checked��ntldrֻ��over com��.��ע��
�������windows�Լ���ntldr.��ô����windbg����ctrl+break�ǲ������õ�.�������ѡ�����ϵͳ�Ľ����ϰ�f10..������break into debugger

��Ȼ�����ѡ��ʹ����д���Ǹ�ntldr(������,��windows�Լ���Ҫ��)

kdfiles����д

    map
    \WINDOWS\system32\DRIVERS\pci.sys
    d:\working\pci\bin\checked\pci.sys


ע���2�е�·��...���ں˼��ص������ǲ�һ����
�㲻��ʹ�� \Systemroot\system32\drivers\pci.sys ������·��

ǿ��Ҫ��ʹ��kdfiles
��Ҫȥ�滻���debuggee�ϵ�pci.sys...
��Ȼϵͳ�����ɱ����û������Ӵ......
