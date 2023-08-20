// File to eval, throws error
print('Hello from test_eval2.js');
print(new Error('test error for traceback (shows filename)').stack);
throw new Error('aiee');
