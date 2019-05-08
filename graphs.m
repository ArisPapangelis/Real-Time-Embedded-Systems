load withoutTimestamps.txt;
load withTimestamps.txt;

%Not using previous Timestamps.
realtimeA = withoutTimestamps(1):0.1:withoutTimestamps(1)+0.1*71999;
A_diff = diff(withoutTimestamps(1:71543));

figure(1);
plot(withoutTimestamps(1:71543));
hold on;
plot(realtimeA);
xlabel('Sample number'), ylabel('Timestamp value');
legend('withoutTimestamps', 'Real time');
hold off;

A_min = min(A_diff);
A_max = max(A_diff);
A_mean = mean(A_diff);
A_median = median(A_diff);
A_std = std(A_diff);

figure(2);
plot(A_diff(69000:70000));
hold on;
X=0.1*ones(1,1000);
plot(X);
xlabel('Sample number'), ylabel('Difference between timestamps');
legend('withoutTimestamps', 'Real time');
hold off;

%Using previous Timestamps.
realtimeB = withTimestamps(1):0.1:withTimestamps(1)+0.1*71999;
B_diff = diff(withTimestamps);

figure(3);
plot(withTimestamps);
hold on;
plot(realtimeB);
xlabel('Sample number'), ylabel('Timestamp value');
legend('withTimestamps', 'Real time');
hold off;

B_min = min(B_diff);
B_max = max(B_diff);
B_mean = mean(B_diff);
B_median = median(B_diff);
B_std = std(B_diff);

figure(4);
plot(B_diff(69000:70000));
hold on;
plot(X);
xlabel('Sample number'), ylabel('Difference between timestamps');
legend('withTimestamps', 'Real time');
hold off;
